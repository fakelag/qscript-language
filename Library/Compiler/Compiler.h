#pragma once

#include "Tokens.h"
#include "AST.h"

namespace Compiler
{
	// TODO: format these APIs
	std::vector< Token_t > Lexer( const std::string& source );
	IASTNode* GenerateIR( const std::vector< Token_t >& tokens );

	// Disassembler
	void DisassembleChunk( const QScript::Chunk_t& chunk, const std::string& identifier );
	int DisassembleInstruction( const QScript::Chunk_t& chunk, int offset );
};
