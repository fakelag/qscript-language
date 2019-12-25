#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Compiler.h"
#include "Instructions.h"

namespace Compiler
{
	void AddDebugSymbol( QScript::Chunk_t* chunk, int start, int lineNr, int colNr, const std::string token )
	{
		if ( lineNr == -1 )
			return;

		chunk->m_Debug.push_back( QScript::Chunk_t::Debug_t{
			start,
			( int ) chunk->m_Code.size(),
			lineNr,
			colNr,
			token,
		} );
	}

	void EmitConstant( QScript::Chunk_t* chunk, const QScript::Value& value, QScript::OpCode shortOpCode, QScript::OpCode longOpCode, Compiler::Assembler& assembler )
	{
		uint32_t constant = 0;

		if ( assembler.OptimizationFlags() & QScript::OF_CONSTANT_STACKING )
		{
			bool found = false;
			for ( uint32_t i = 0; i < chunk->m_Constants.size(); ++i )
			{
				if ( chunk->m_Constants[ i ] == value )
				{
					constant = i;
					found = true;
					break;
				}
			}

			if ( !found )
				constant = AddConstant( value, chunk );
		}
		else
		{
			constant = AddConstant( value, chunk );
		}

		if ( constant > 255 )
		{
			EmitByte( longOpCode, chunk );
			EmitByte( ENCODE_LONG( constant, 0 ), chunk );
			EmitByte( ENCODE_LONG( constant, 1 ), chunk );
			EmitByte( ENCODE_LONG( constant, 2 ), chunk );
			EmitByte( ENCODE_LONG( constant, 3 ), chunk );
		}
		else
		{
			EmitByte( shortOpCode, chunk );
			EmitByte( ( uint8_t ) constant, chunk );
		}
	}

	void RequireAssignability( BaseNode* node )
	{
		switch ( node->Id() )
		{
		case NODE_NAME:
			break;
		default:
			throw CompilerException( "cp_invalid_assign_target", "Invalid assignment target", node->LineNr(), node->ColNr(), node->Token() );
		}
	}

	BaseNode::BaseNode( int lineNr, int colNr, const std::string token, NodeType type, NodeId id )
	{
		m_LineNr		= lineNr;
		m_ColNr			= colNr;
		m_Token			= token;
		m_NodeType		= type;
		m_NodeId		= id;
	}

	TermNode::TermNode( int lineNr, int colNr, const std::string token, NodeId id )
		: BaseNode( lineNr, colNr, token, NT_TERM, id )
	{
	}

	void TermNode::Compile( Assembler& assembler, uint32_t options )
	{
		auto chunk = assembler.CurrentChunk();

		// Call CompileXXX functions from Compiler.cpp for code generation,
		// but append debugging info to the chunk here
		int start = chunk->m_Code.size();

		switch( m_NodeId )
		{
		case NODE_RETURN: EmitByte( QScript::OpCode::OP_RETN, chunk ); break;
		default:
			throw CompilerException( "cp_invalid_term_node", "Unknown terminating node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}

		AddDebugSymbol( chunk, start, m_LineNr, m_ColNr, m_Token );
	}

	ValueNode::ValueNode( int lineNr, int colNr, const std::string token, NodeId id, const QScript::Value& value )
		: BaseNode( lineNr, colNr, token, NT_VALUE, id )
	{
		m_Value.From( value );
	}

	void ValueNode::Compile( Assembler& assembler, uint32_t options )
	{
		auto chunk = assembler.CurrentChunk();
		int start = chunk->m_Code.size();

		switch ( m_NodeId )
		{
		case NODE_NAME:
		{
			uint32_t local;
			if ( assembler.FindLocal( AS_STRING( m_Value )->GetString(), &local ) )
			{
				if ( local > 255 )
				{
					EmitByte( ( options & CO_ASSIGN ) ? QScript::OpCode::OP_SL_LONG : QScript::OpCode::OP_LL_LONG, chunk );
					EmitByte( ENCODE_LONG( local, 0 ), chunk );
					EmitByte( ENCODE_LONG( local, 1 ), chunk );
					EmitByte( ENCODE_LONG( local, 2 ), chunk );
					EmitByte( ENCODE_LONG( local, 3 ), chunk );
				}
				else
				{
					EmitByte( ( options & CO_ASSIGN ) ? QScript::OpCode::OP_SL_SHORT : QScript::OpCode::OP_LL_SHORT, chunk );
					EmitByte( ( uint8_t ) local, chunk );
				}

				if ( options & CO_ASSIGN )
					EmitByte( QScript::OpCode::OP_POP, chunk );
			}
			else
			{
				if ( options & CO_ASSIGN )
					EmitConstant( chunk, m_Value, QScript::OpCode::OP_SG_SHORT, QScript::OpCode::OP_SG_LONG, assembler );
				else
					EmitConstant( chunk, m_Value, QScript::OpCode::OP_LG_SHORT, QScript::OpCode::OP_LG_LONG, assembler );
			}

			break;
		}
		default:
		{
			if ( IS_NULL( m_Value ) )
				EmitByte( QScript::OP_PNULL, chunk );
			else
				EmitConstant( chunk, m_Value, QScript::OpCode::OP_LD_SHORT, QScript::OpCode::OP_LD_LONG, assembler );
		}
		}

		AddDebugSymbol( chunk, start, m_LineNr, m_ColNr, m_Token );
	}

	bool ValueNode::IsString() const
	{
		return IS_STRING( m_Value );
	}

	ComplexNode::ComplexNode( int lineNr, int colNr, const std::string token, NodeId id, BaseNode* left, BaseNode* right )
		: BaseNode( lineNr, colNr, token, NT_COMPLEX, id )
	{
		m_Left = left;
		m_Right = right;
	}

	void ComplexNode::Release()
	{
		if ( m_Left )
		{
			m_Left->Release();
			delete m_Left;
		}

		if ( m_Right )
		{
			m_Right->Release();
			delete m_Right;
		}
	}

	void ComplexNode::Compile( Assembler& assembler, uint32_t options )
	{
		int start = -1;
		auto chunk = assembler.CurrentChunk();

		if ( m_NodeId == NODE_ASSIGN )
		{
			RequireAssignability( m_Left );

			// When assigning a value, first evaluate right hand operand (value)
			// and then set it via the left hand operand
			m_Right->Compile( assembler, options );
			m_Left->Compile( assembler, options | CO_ASSIGN );
		}
		else if ( m_NodeId == NODE_VAR )
		{
			// Target must be assignable
			RequireAssignability( m_Left );

			auto& varName = static_cast< ValueNode* >( m_Left )->GetValue();

			if ( m_Right )
			{
				// Assign variable at declaration
				m_Right->Compile( assembler, options );
				if ( assembler.StackDepth() > 0 )
					assembler.CreateLocal( AS_STRING( varName )->GetString() );
				else
					m_Left->Compile( assembler, options | CO_ASSIGN );
			}
			else
			{
				// Empty variable
				start = chunk->m_Code.size();
				EmitByte( QScript::OpCode::OP_PNULL, chunk );

				if ( assembler.StackDepth() == 0 )
				{
					// Global variable
					EmitConstant( chunk, varName, QScript::OpCode::OP_SG_SHORT, QScript::OpCode::OP_SG_LONG, assembler );
				}
				else
				{
					// Local variable
					assembler.CreateLocal( AS_STRING( varName )->GetString() );
				}
			}
		}
		else
		{
			m_Left->Compile( assembler, options );
			m_Right->Compile( assembler, options );

			start = chunk->m_Code.size();

			std::map< NodeId, QScript::OpCode > singleByte ={
				{ NODE_ADD, 			QScript::OpCode::OP_ADD },
				{ NODE_SUB, 			QScript::OpCode::OP_SUB },
				{ NODE_MUL, 			QScript::OpCode::OP_MUL },
				{ NODE_DIV, 			QScript::OpCode::OP_DIV },
				{ NODE_EQUALS,			QScript::OpCode::OP_EQ },
				{ NODE_NOTEQUALS,		QScript::OpCode::OP_NEQ },
				{ NODE_GREATERTHAN,		QScript::OpCode::OP_GT },
				{ NODE_GREATEREQUAL,	QScript::OpCode::OP_GTE },
				{ NODE_LESSTHAN,		QScript::OpCode::OP_LT },
				{ NODE_LESSEQUAL,		QScript::OpCode::OP_LTE },
			};

			auto opCode = singleByte.find( m_NodeId );
			if ( opCode != singleByte.end() )
				EmitByte( opCode->second, chunk );
			else
				throw CompilerException( "cp_invalid_complex_node", "Unknown complex node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}

		if ( start != -1 )
			AddDebugSymbol( chunk, start, m_LineNr, m_ColNr, m_Token );
	}

	SimpleNode::SimpleNode( int lineNr, int colNr, const std::string token, NodeId id, BaseNode* node )
		: BaseNode( lineNr, colNr, token, NT_SIMPLE, id )
	{
		m_Node = node;
	}

	void SimpleNode::Release()
	{
		if ( m_Node )
		{
			m_Node->Release();
			delete m_Node;
		}
	}

	void SimpleNode::Compile( Assembler& assembler, uint32_t options )
	{
		auto chunk = assembler.CurrentChunk();
		int start = -1;

		std::map< NodeId, QScript::OpCode > singleByte ={
			{ NODE_PRINT, 			QScript::OpCode::OP_PRINT },
			{ NODE_RETURN, 			QScript::OpCode::OP_RETN },
			{ NODE_NOT, 			QScript::OpCode::OP_NOT },
			{ NODE_NEG, 			QScript::OpCode::OP_NEG },
			{ NODE_POP,				QScript::OpCode::OP_POP },
		};

		auto opCode = singleByte.find( m_NodeId );
		if ( opCode != singleByte.end() )
		{
			m_Node->Compile( assembler, options );

			start = chunk->m_Code.size();
			EmitByte( opCode->second, chunk );
		}
		else
		{
			throw Exception( "cp_invalid_simple_node", "Unknown simple node: " + std::to_string( m_NodeId ) );
		}

		if ( start != -1 )
			AddDebugSymbol( chunk, start, m_LineNr, m_ColNr, m_Token );
	}

	ListNode::ListNode( int lineNr, int colNr, const std::string token, NodeId id, const std::vector< BaseNode* >& nodeList )
		: BaseNode( lineNr, colNr, token, NT_SIMPLE, id )
	{
		m_NodeList = nodeList;
	}

	void ListNode::Release()
	{
		for ( auto node : m_NodeList )
		{
			node->Release();
			delete node;
		}

		m_NodeList.clear();
	}

	void ListNode::Compile( Assembler& assembler, uint32_t options )
	{
		if ( m_NodeId == NODE_SCOPE )
			assembler.PushScope();

		for ( auto node : m_NodeList )
			node->Compile( assembler, options );

		if ( m_NodeId == NODE_SCOPE )
		{
			// Clear stack
			for ( int i = assembler.LocalCount() - 1; i >= 0; --i )
				EmitByte( QScript::OpCode::OP_POP, assembler.CurrentChunk() );

			assembler.PopScope();
		}
	}
}
