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

	void EmitConstant( QScript::Chunk_t* chunk, const QScript::Value& value, QScript::OpCode shortOpCode, QScript::OpCode longOpCode )
	{
		if ( chunk->m_Constants.size() >= 255 )
		{
			auto longIndex = ( uint32_t ) AddConstant( value, chunk );

			EmitByte( longOpCode, chunk );
			EmitByte( ENCODE_LONG( longIndex, 0 ), chunk );
			EmitByte( ENCODE_LONG( longIndex, 1 ), chunk );
			EmitByte( ENCODE_LONG( longIndex, 2 ), chunk );
			EmitByte( ENCODE_LONG( longIndex, 3 ), chunk );
		}
		else
		{
			auto shortIndex = ( uint8_t ) AddConstant( value, chunk );

			EmitByte( shortOpCode, chunk );
			EmitByte( shortIndex, chunk );
		}
	}

	void RequireAssignability( BaseNode* node )
	{
		switch ( node->Id() )
		{
		case NODE_NAME:
		case NODE_VAR:
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

	void TermNode::Compile( QScript::Chunk_t* chunk, uint32_t options )
	{
		// Call CompileXXX functions from Compiler.cpp for code generation,
		// but append debugging info to the chunk here
		int start = chunk->m_Code.size();

		switch( m_NodeId )
		{
		case NODE_POP: EmitByte( QScript::OpCode::OP_POP, chunk ); break;
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

	void ValueNode::Compile( QScript::Chunk_t* chunk, uint32_t options )
	{
		int start = chunk->m_Code.size();

		switch ( m_NodeId )
		{
		case NODE_NAME:
		{
			if ( options & CO_ASSIGN )
				EmitConstant( chunk, m_Value, QScript::OpCode::OP_SG_SHORT, QScript::OpCode::OP_SG_LONG );
			else
				EmitConstant( chunk, m_Value, QScript::OpCode::OP_LG_SHORT, QScript::OpCode::OP_LG_LONG );
			break;
		}
		default:
		{
			if ( IS_NULL( m_Value ) )
				EmitByte( QScript::OP_PNULL, chunk );
			else
				EmitConstant( chunk, m_Value, QScript::OpCode::OP_LD_SHORT, QScript::OpCode::OP_LD_LONG );
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
		delete m_Left;
		delete m_Right;
	}

	void ComplexNode::Compile( QScript::Chunk_t* chunk, uint32_t options )
	{
		if ( m_NodeId == NODE_ASSIGN )
		{
			RequireAssignability( m_Left );

			// When assigning a value, first evaluate right hand operant (value)
			// and then set it via the left hand operand
			m_Right->Compile( chunk, options );
			m_Left->Compile( chunk, options | CO_ASSIGN );
		}
		else
		{
			m_Left->Compile( chunk, options );
			m_Right->Compile( chunk, options );

			int start = chunk->m_Code.size();

			std::map< Compiler::NodeId, QScript::OpCode > singleByte ={
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

			AddDebugSymbol( chunk, start, m_LineNr, m_ColNr, m_Token );
		}
	}

	SimpleNode::SimpleNode( int lineNr, int colNr, const std::string token, NodeId id, BaseNode* node )
		: BaseNode( lineNr, colNr, token, NT_SIMPLE, id )
	{
		m_Node = node;
	}

	void SimpleNode::Release()
	{
		delete m_Node;
	}

	void SimpleNode::Compile( QScript::Chunk_t* chunk, uint32_t options )
	{
		int start = 0;

		std::map< Compiler::NodeId, QScript::OpCode > singleByte ={
			{ NODE_PRINT, 			QScript::OpCode::OP_PRINT },
			{ NODE_RETURN, 			QScript::OpCode::OP_RETN },
			{ NODE_NOT, 			QScript::OpCode::OP_NOT },
			{ NODE_NEG, 			QScript::OpCode::OP_NEG },
		};

		auto opCode = singleByte.find( m_NodeId );
		if ( opCode != singleByte.end() )
		{
			m_Node->Compile( chunk, options );

			start = chunk->m_Code.size();
			EmitByte( opCode->second, chunk );
		}
		else
		{
			switch ( m_NodeId )
			{
			case NODE_VAR:
			{
				// Define a variable (empty or otherwise)
				if ( options & CO_ASSIGN )
				{
					RequireAssignability( m_Node );
					m_Node->Compile( chunk, options );
				}
				else
				{
					// Undefined variable
					auto& value = static_cast< ValueNode* >( m_Node )->GetValue();

					EmitByte( QScript::OpCode::OP_PNULL, chunk );
					EmitConstant( chunk, value, QScript::OpCode::OP_SG_SHORT, QScript::OpCode::OP_SG_LONG );
				}

				break;
			}
			default:
				throw Exception( "cp_invalid_simple_node", "Unknown simple node: " + std::to_string( m_NodeId ) );
			}
		}

		AddDebugSymbol( chunk, start, m_LineNr, m_ColNr, m_Token );
	}
}
