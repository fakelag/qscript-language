#include "../Includes/QLibPCH.h"
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

		// Return compiled code
		return chunk;
	}
}
