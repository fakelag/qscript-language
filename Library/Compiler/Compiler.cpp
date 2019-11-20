#include "../Includes/QLibPCH.h"
#include "../Includes/Instructions.h"
#include "Compiler.h"

namespace QScript
{
	Chunk_t Compile( const char* pszSource )
	{
		Chunk_t chunk;

		// Lexical analysis (tokenization)
		std::string source( pszSource );
		auto tokens = Compiler::Lexer( source );

		// Generate IR

		// Run IR optimizers

		// Compile bytecode
		chunk.m_Code.push_back( OpCode::OP_RETN );
		chunk.m_Code.push_back( OpCode::OP_RETN );
		chunk.m_Code.push_back( OpCode::OP_RETN );

		// Return compiled code
		return chunk;
	}
}
