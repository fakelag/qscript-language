#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Compiler.h"
#include "Instructions.h"

#define COMPILE_EXPRESSION( options ) (options | CO_EXPRESSION)
#define COMPILE_STATEMENT( options ) (options & ~CO_EXPRESSION)
#define COMPILE_ASSIGN_TARGET( options ) (options | CO_ASSIGN)

#define IS_STATEMENT( options ) (!(options & CO_EXPRESSION))

#define EXPECTED_EXPRESSION CompilerException( "cp_expected_expression", "Expected an expression, got: + \"" + m_Token + "\" (statement)", m_LineNr, m_ColNr, m_Token );
#define EXPECTED_STATEMENT CompilerException( "cp_expected_statement", "Expected a statement, got: + \"" + m_Token + "\" (expression)", m_LineNr, m_ColNr, m_Token );

namespace Compiler
{
	void AddDebugSymbol( QScript::Chunk_t* chunk, uint32_t start, int lineNr, int colNr, const std::string token )
	{
		if ( lineNr == -1 )
			return;

		chunk->m_Debug.push_back( QScript::Chunk_t::Debug_t{
			start,
			( uint32_t ) chunk->m_Code.size(),
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

	void RelocateDebugSymbols( QScript::Chunk_t* chunk, uint32_t from, uint32_t patchSize )
	{
		for ( auto& symbol : chunk->m_Debug )
		{
			if ( symbol.m_From > from )
				symbol.m_From += patchSize;

			if ( symbol.m_To > from )
				symbol.m_To += patchSize;
		}
	}

	uint32_t PlaceJump( QScript::Chunk_t* chunk, uint32_t from, uint32_t size, QScript::OpCode shortOpCode, QScript::OpCode longOpCode )
	{
		bool isLong = size > 255;
		short patchSize = isLong ? 5 : 2;

		std::vector< uint8_t > body;
		body.insert( body.begin(), chunk->m_Code.begin() + from, chunk->m_Code.end() );

		// Append patchSize additional bytes for jump
		chunk->m_Code.resize( chunk->m_Code.size() + patchSize, 0xFF );

		// Move body by patchSize
		for ( size_t i = 0; i < body.size(); ++i )
			chunk->m_Code[ from + i + patchSize ] = body[ i ];

		if ( isLong )
		{
			// Write long jump
			chunk->m_Code[ from ] = longOpCode;
			chunk->m_Code[ from + 1 ] = ENCODE_LONG( size, 0 );
			chunk->m_Code[ from + 2 ] = ENCODE_LONG( size, 1 );
			chunk->m_Code[ from + 3 ] = ENCODE_LONG( size, 2 );
			chunk->m_Code[ from + 4 ] = ENCODE_LONG( size, 3 );
		}
		else
		{
			// Patch single-byte jump
			chunk->m_Code[ from ] = shortOpCode;
			chunk->m_Code[ from + 1 ] = ( uint8_t ) size;
		}

		RelocateDebugSymbols( chunk, from, patchSize );

		return patchSize;
	}

	void PatchJump( QScript::Chunk_t* chunk, uint32_t from, uint32_t newSize, QScript::OpCode shortOpCode, QScript::OpCode longOpCode )
	{
		if ( chunk->m_Code[ from ] == shortOpCode )
		{
			if ( newSize > 255 )
				throw Exception( "cp_invalid_jmp_patch_size", "Expected long JMP instruction, got short" );

			chunk->m_Code[ from + 1 ] = ( uint8_t ) newSize;
		}
		else if ( chunk->m_Code[ from ] == longOpCode )
		{
			chunk->m_Code[ from + 1 ] = ENCODE_LONG( newSize, 0 );
			chunk->m_Code[ from + 2 ] = ENCODE_LONG( newSize, 1 );
			chunk->m_Code[ from + 3 ] = ENCODE_LONG( newSize, 2 );
			chunk->m_Code[ from + 4 ] = ENCODE_LONG( newSize, 3 );
		}
		else
		{
			throw Exception( "cp_invalid_jmp_patch_inst", "Expected JMP instruction at patch site" );
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
		uint32_t start = chunk->m_Code.size();

		switch( m_NodeId )
		{
		case NODE_RETURN: EmitByte( QScript::OpCode::OP_RETURN, chunk ); break;
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
		uint32_t start = chunk->m_Code.size();

		switch ( m_NodeId )
		{
		case NODE_NAME:
		{
			uint32_t local;
			if ( assembler.FindLocal( AS_STRING( m_Value )->GetString(), &local ) )
			{
				if ( local > 255 )
				{
					EmitByte( ( options & CO_ASSIGN ) ? QScript::OpCode::OP_SET_LOCAL_LONG : QScript::OpCode::OP_LOAD_LOCAL_LONG, chunk );
					EmitByte( ENCODE_LONG( local, 0 ), chunk );
					EmitByte( ENCODE_LONG( local, 1 ), chunk );
					EmitByte( ENCODE_LONG( local, 2 ), chunk );
					EmitByte( ENCODE_LONG( local, 3 ), chunk );
				}
				else
				{
					EmitByte( ( options & CO_ASSIGN ) ? QScript::OpCode::OP_SET_LOCAL_SHORT : QScript::OpCode::OP_LOAD_LOCAL_SHORT, chunk );
					EmitByte( ( uint8_t ) local, chunk );
				}
			}
			else
			{
				if ( options & CO_ASSIGN )
					EmitConstant( chunk, m_Value, QScript::OpCode::OP_SET_GLOBAL_SHORT, QScript::OpCode::OP_SET_GLOBAL_LONG, assembler );
				else
					EmitConstant( chunk, m_Value, QScript::OpCode::OP_LOAD_GLOBAL_SHORT, QScript::OpCode::OP_LOAD_GLOBAL_LONG, assembler );
			}

			if ( IS_STATEMENT( options ) && ( options & CO_ASSIGN ) )
				EmitByte( QScript::OpCode::OP_POP, chunk );

			break;
		}
		default:
		{
			if ( IS_STATEMENT( options ) )
				throw EXPECTED_STATEMENT;

			if ( IS_NULL( m_Value ) )
				EmitByte( QScript::OP_LOAD_NULL, chunk );
			else
				EmitConstant( chunk, m_Value, QScript::OpCode::OP_LOAD_CONSTANT_SHORT, QScript::OpCode::OP_LOAD_CONSTANT_LONG, assembler );
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
		auto chunk = assembler.CurrentChunk();
		uint32_t start = chunk->m_Code.size();

		switch ( m_NodeId )
		{
		case NODE_ASSIGN:
		{
			RequireAssignability( m_Left );

			// When assigning a value, first evaluate right hand operand (value)
			// and then set it via the left hand operand
			m_Right->Compile( assembler, COMPILE_EXPRESSION( options ) );
			m_Left->Compile( assembler, COMPILE_ASSIGN_TARGET( options ) );
			break;
		}
		case NODE_AND:
		{
			m_Left->Compile( assembler, COMPILE_EXPRESSION( options ) );

			uint32_t endJump = chunk->m_Code.size();

			// Left hand operand evaluated to true, discard it and return the right hand result
			EmitByte( QScript::OpCode::OP_POP, chunk );

			m_Right->Compile( assembler, COMPILE_EXPRESSION( options ) );

			// Create jump instruction
			PlaceJump( chunk, endJump, chunk->m_Code.size() - endJump, QScript::OpCode::OP_JUMP_IF_ZERO_SHORT, QScript::OpCode::OP_JUMP_IF_ZERO_LONG );
			break;
		}
		case NODE_OR:
		{
			m_Left->Compile( assembler, COMPILE_EXPRESSION( options ) );

			uint32_t endJump = chunk->m_Code.size();

			// Pop left operand off
			EmitByte( QScript::OpCode::OP_POP, chunk );

			m_Right->Compile( assembler, COMPILE_EXPRESSION( options ) );

			uint32_t patchSize = PlaceJump( chunk, endJump, chunk->m_Code.size() - endJump, QScript::OpCode::OP_JUMP_SHORT, QScript::OpCode::OP_JUMP_LONG );

			PlaceJump( chunk, endJump, patchSize, QScript::OpCode::OP_JUMP_IF_ZERO_SHORT, QScript::OpCode::OP_JUMP_IF_ZERO_LONG );
			break;
		}
		default:
		{
			m_Left->Compile( assembler, COMPILE_EXPRESSION( options ) );
			m_Right->Compile( assembler, COMPILE_EXPRESSION( options ) );

			// Note: All the opcodes listed here MUST use both left and right hand values
			std::map< NodeId, QScript::OpCode > singleByte ={
				{ NODE_ADD, 			QScript::OpCode::OP_ADD },
				{ NODE_SUB, 			QScript::OpCode::OP_SUB },
				{ NODE_MUL, 			QScript::OpCode::OP_MUL },
				{ NODE_DIV, 			QScript::OpCode::OP_DIV },
				{ NODE_EQUALS,			QScript::OpCode::OP_EQUALS },
				{ NODE_NOTEQUALS,		QScript::OpCode::OP_NOT_EQUALS },
				{ NODE_GREATERTHAN,		QScript::OpCode::OP_GREATERTHAN },
				{ NODE_GREATEREQUAL,	QScript::OpCode::OP_GREATERTHAN_OR_EQUAL },
				{ NODE_LESSTHAN,		QScript::OpCode::OP_LESSTHAN },
				{ NODE_LESSEQUAL,		QScript::OpCode::OP_LESSTHAN_OR_EQUAL },
			};

			auto opCode = singleByte.find( m_NodeId );
			if ( opCode != singleByte.end() )
				EmitByte( opCode->second, chunk );
			else
				throw CompilerException( "cp_invalid_complex_node", "Unknown complex node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );

			break;
		}
		}

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
		uint32_t start = chunk->m_Code.size();

		std::map< NodeId, QScript::OpCode > singleByte ={
			{ NODE_PRINT, 			QScript::OpCode::OP_PRINT },
			{ NODE_RETURN, 			QScript::OpCode::OP_RETURN },
			{ NODE_NOT, 			QScript::OpCode::OP_NOT },
			{ NODE_NEG, 			QScript::OpCode::OP_NEGATE },
		};

		auto opCode = singleByte.find( m_NodeId );
		if ( opCode != singleByte.end() )
		{
			m_Node->Compile( assembler, COMPILE_EXPRESSION( options ) );
			EmitByte( opCode->second, chunk );
		}
		else
		{
			throw CompilerException( "cp_invalid_simple_node", "Unknown simple node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}

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
			if ( node )
			{
				node->Release();
				delete node;
			}
		}

		m_NodeList.clear();
	}

	void ListNode::Compile( Assembler& assembler, uint32_t options )
	{
		auto chunk = assembler.CurrentChunk();
		uint32_t start = chunk->m_Code.size();

		switch ( m_NodeId )
		{
		case NODE_IF:
		{
			if ( !IS_STATEMENT( options ) )
				throw EXPECTED_EXPRESSION;

			// Compile condition, now the result is at the top of the stack
			m_NodeList[ 0 ]->Compile( assembler, COMPILE_EXPRESSION( options ) );

			// Address of the next instruction from the jump
			uint32_t thenBodyBegin = chunk->m_Code.size();

			// Pop condition value off stack
			EmitByte( QScript::OpCode::OP_POP, chunk );

			// Compile body
			m_NodeList[ 1 ]->Compile( assembler, COMPILE_STATEMENT( options ) );

			uint32_t elseBodyBegin = chunk->m_Code.size();

			// Pop condition value off stack
			EmitByte( QScript::OpCode::OP_POP, chunk );

			// Compile optional else-branch
			if ( m_NodeList[ 2 ] )
				m_NodeList[ 2 ]->Compile( assembler, COMPILE_STATEMENT( options ) );

			uint32_t elseBodyEnd = chunk->m_Code.size();

			// Jump over else branch
			elseBodyBegin += PlaceJump( chunk, elseBodyBegin, elseBodyEnd - elseBodyBegin, QScript::OpCode::OP_JUMP_SHORT, QScript::OpCode::OP_JUMP_LONG );

			// Create jump instruction
			PlaceJump( chunk, thenBodyBegin, elseBodyBegin - thenBodyBegin, QScript::OpCode::OP_JUMP_IF_ZERO_SHORT, QScript::OpCode::OP_JUMP_IF_ZERO_LONG );
			break;
		}
		case NODE_SCOPE:
		{
			if ( !IS_STATEMENT( options ) )
				throw EXPECTED_EXPRESSION;

			if ( m_NodeId == NODE_SCOPE )
				assembler.PushScope();

			// Compile each statement in scope body
			for ( auto node : m_NodeList )
				node->Compile( assembler, COMPILE_STATEMENT( options ) );

			if ( m_NodeId == NODE_SCOPE )
			{
				// Clear stack
				for ( int i = assembler.LocalsInCurrentScope() - 1; i >= 0; --i )
					EmitByte( QScript::OpCode::OP_POP, assembler.CurrentChunk() );

				assembler.PopScope();
			}

			break;
		}
		case NODE_VAR:
		{
			// Target must be assignable
			RequireAssignability( m_NodeList[ 0 ] );

			auto& varName = static_cast< ValueNode* >( m_NodeList[ 0 ] )->GetValue();

			if ( m_NodeList[ 1 ] )
			{
				// Assign variable at declaration
				m_NodeList[ 1 ]->Compile( assembler, COMPILE_EXPRESSION( options ) );

				if ( assembler.StackDepth() > 0 )
					assembler.CreateLocal( AS_STRING( varName )->GetString() );
				else
					m_NodeList[ 0 ]->Compile( assembler, COMPILE_ASSIGN_TARGET( options ) );
			}
			else
			{
				if ( !IS_STATEMENT( options ) )
					throw EXPECTED_EXPRESSION;

				// Empty variable
				EmitByte( QScript::OpCode::OP_LOAD_NULL, chunk );

				if ( assembler.StackDepth() == 0 )
				{
					// Global variable
					EmitConstant( chunk, varName, QScript::OpCode::OP_SET_GLOBAL_SHORT, QScript::OpCode::OP_SET_GLOBAL_LONG, assembler );
					EmitByte( QScript::OpCode::OP_POP, chunk );
				}
				else
				{
					// Local variable
					assembler.CreateLocal( AS_STRING( varName )->GetString() );
				}
			}
			break;
		}
		case NODE_WHILE:
		{
			if ( !IS_STATEMENT( options ) )
				throw EXPECTED_EXPRESSION;

			uint32_t loopConditionBegin = chunk->m_Code.size();

			// Compile condition
			m_NodeList[ 0 ]->Compile( assembler, COMPILE_EXPRESSION( options ) );

			uint32_t loopBodyBegin = chunk->m_Code.size();

			EmitByte( QScript::OpCode::OP_POP, chunk );

			// Compile body
			m_NodeList[ 1 ]->Compile( assembler, COMPILE_STATEMENT( options ) );

			uint32_t firstJumpSize = chunk->m_Code.size() - loopBodyBegin;
			PlaceJump( chunk, loopBodyBegin, firstJumpSize + 5, QScript::OpCode::OP_JUMP_IF_ZERO_SHORT, QScript::OpCode::OP_JUMP_IF_ZERO_LONG );

			uint32_t patchSize = PlaceJump( chunk, chunk->m_Code.size(), chunk->m_Code.size() - loopConditionBegin, QScript::OpCode::OP_JUMP_BACK_SHORT, QScript::OpCode::OP_JUMP_BACK_LONG );

			PatchJump( chunk, loopBodyBegin, firstJumpSize + patchSize, QScript::OpCode::OP_JUMP_IF_ZERO_SHORT, QScript::OpCode::OP_JUMP_IF_ZERO_LONG );

			EmitByte( QScript::OpCode::OP_POP, chunk );
			break;
		}
		default:
			throw CompilerException( "cp_invalid_list_node", "Unknown list node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}

		AddDebugSymbol( chunk, start, m_LineNr, m_ColNr, m_Token );
	}
}
