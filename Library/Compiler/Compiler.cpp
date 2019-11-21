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
		chunk.m_Constants.push_back( 5.6 );

		chunk.m_Code.push_back( OpCode::OP_CNST );
		chunk.m_Code.push_back( chunk.m_Constants.size() - 1 );
		chunk.m_Code.push_back( OpCode::OP_RETN );

		// Return compiled code
		return chunk;
	}
}
