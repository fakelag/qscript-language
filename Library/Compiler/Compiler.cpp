#include "QLibPCH.h"
#include "Instructions.h"
#include "Compiler.h"
#include "Exception.h"

namespace QScript
{
	Chunk_t Compile( const char* source )
	{
		Chunk_t chunk;

		// Lexical analysis (tokenization)
		auto tokens = Compiler::Lexer( std::string( source ) );

		// Generate IR
		auto entryNodes = Compiler::GenerateIR( tokens );

		// Run IR optimizers

		// Compile bytecode
#if 0
		for ( auto node : entryNodes )
			node->Compile( &chunk );
#else
		// LOAD 0 (5.5)
		chunk.m_Constants.push_back( 5.5 );
		chunk.m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk.m_Code.push_back( ( uint8_t ) chunk.m_Constants.size() - 1 );

		// LOAD 1 (2.7)
		chunk.m_Constants.push_back( 2.7 );
		chunk.m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk.m_Code.push_back( ( uint8_t ) chunk.m_Constants.size() - 1 );

		// ADD
		chunk.m_Code.push_back( QScript::OpCode::OP_ADD );

		// LOAD 2 (2.0)
		chunk.m_Constants.push_back( 2.0 );
		chunk.m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk.m_Code.push_back( ( uint8_t ) chunk.m_Constants.size() - 1 );

		// SUB
		chunk.m_Code.push_back( QScript::OpCode::OP_ADD );

		// LOAD 2 (5.0)
		chunk.m_Constants.push_back( 5.0 );
		chunk.m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk.m_Code.push_back( ( uint8_t ) chunk.m_Constants.size() - 1 );

		// MUL
		chunk.m_Code.push_back( QScript::OpCode::OP_MUL );


		// LOAD 1.5 (5.0)
		chunk.m_Constants.push_back( 1.5 );
		chunk.m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk.m_Code.push_back( ( uint8_t ) chunk.m_Constants.size() - 1 );

		// DIV
		chunk.m_Code.push_back( QScript::OpCode::OP_DIV );

		// NEG
		chunk.m_Code.push_back( QScript::OpCode::OP_NEG );

		// RET
		chunk.m_Code.push_back( QScript::OpCode::OP_RETN );
#endif

		// Return compiled code
		return chunk;
	}
}

namespace Compiler
{
	void CompileValueNode( const QScript::Value& value, QScript::Chunk_t* chunk )
	{
		chunk->m_Constants.push_back( value );
		chunk->m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk->m_Code.push_back( ( uint8_t ) chunk->m_Constants.size() - 1 );
	}

	void CompileTermNode( NodeId nodeId, QScript::Chunk_t* chunk )
	{
		switch ( nodeId )
		{
		case NODE_RETURN:
			chunk->m_Code.push_back( QScript::OpCode::OP_RETN );
			break;
		default:
			throw Exception( "cp_invalid_term_node", "Unknown terminating node: " + std::to_string( nodeId ) );
		}
	}
}
