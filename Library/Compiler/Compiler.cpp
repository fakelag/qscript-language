#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Instructions.h"
#include "Compiler.h"
#include "Exception.h"

#if 1
#include "../Common/Value.h"
#endif

namespace QScript
{
	Chunk_t* Compile( const char* source )
	{
		Chunk_t* chunk = AllocChunk();

		// Lexical analysis (tokenization)
		auto tokens = Compiler::Lexer( std::string( source ) );

		// Generate IR
		auto entryNodes = Compiler::GenerateIR( tokens );

		// Run IR optimizers

		// Compile bytecode
#if 0
		for ( auto node : entryNodes )
			node->Compile( chunk );

		for ( auto node : entryNodes )
			delete node;

		entryNodes.clear();
#else
		/*chunk->m_Constants.push_back( MAKE_NUMBER( 5.5 ) );
		chunk->m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk->m_Code.push_back( ( uint8_t ) chunk->m_Constants.size() - 1 );

		chunk->m_Constants.push_back( MAKE_NUMBER( 2.7 ) );
		chunk->m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk->m_Code.push_back( ( uint8_t ) chunk->m_Constants.size() - 1 );

		chunk->m_Code.push_back( QScript::OpCode::OP_EQ );

		chunk->m_Constants.push_back( MAKE_NUMBER( 5.5 ) );
		chunk->m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk->m_Code.push_back( ( uint8_t ) chunk->m_Constants.size() - 1 );

		chunk->m_Constants.push_back( MAKE_NUMBER( 2.7 ) );
		chunk->m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk->m_Code.push_back( ( uint8_t ) chunk->m_Constants.size() - 1 );

		chunk->m_Code.push_back( QScript::OpCode::OP_NEQ );

		chunk->m_Constants.push_back( MAKE_NUMBER( 5.5 ) );
		chunk->m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk->m_Code.push_back( ( uint8_t ) chunk->m_Constants.size() - 1 );

		chunk->m_Constants.push_back( MAKE_NUMBER( 5.5 ) );
		chunk->m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk->m_Code.push_back( ( uint8_t ) chunk->m_Constants.size() - 1 );

		chunk->m_Code.push_back( QScript::OpCode::OP_EQ );*/

		chunk->m_Constants.push_back( MAKE_BOOL( true ) );
		chunk->m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk->m_Code.push_back( ( uint8_t ) chunk->m_Constants.size() - 1 );

		chunk->m_Constants.push_back( MAKE_BOOL( true ) );
		chunk->m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk->m_Code.push_back( ( uint8_t ) chunk->m_Constants.size() - 1 );

		chunk->m_Code.push_back( QScript::OpCode::OP_EQ );

		/*chunk->m_Constants.push_back( MAKE_BOOL( false ) );
		chunk->m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk->m_Code.push_back( ( uint8_t ) chunk->m_Constants.size() - 1 );

		chunk->m_Constants.push_back( MAKE_BOOL( true ) );
		chunk->m_Code.push_back( QScript::OpCode::OP_LOAD );
		chunk->m_Code.push_back( ( uint8_t ) chunk->m_Constants.size() - 1 );

		chunk->m_Code.push_back( QScript::OpCode::OP_EQ );*/

		// RET
		chunk->m_Code.push_back( QScript::OpCode::OP_RETN );
#endif

		// Return compiled code
		return chunk;
	}
}

namespace Compiler
{
	uint8_t AddConstant( const QScript::Value& value, QScript::Chunk_t* chunk )
	{
		chunk->m_Constants.push_back( value );
		return ( uint8_t ) chunk->m_Constants.size() - 1;
	}

	void EmitByte( uint8_t byte, QScript::Chunk_t* chunk )
	{
		chunk->m_Code.push_back( byte );
	}
}
