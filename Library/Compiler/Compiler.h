#pragma once

#include "Tokens.h"
#include "AST.h"

namespace Compiler
{
	std::vector< Token_t > Lexer( const std::string& source );
	std::vector< BaseNode* > GenerateIR( const std::vector< Token_t >& tokens );
	// std::vector< BaseNode* > OptimizeIR( std::vector< BaseNode* > nodes );

	// Compilers
	void CompileValueNode( const QScript::Value& value, QScript::Chunk_t* chunk );
	void CompileTermNode( NodeId nodeId, QScript::Chunk_t* chunk );

	// Disassembler
	void DisassembleChunk( const QScript::Chunk_t& chunk, const std::string& identifier );
	int DisassembleInstruction( const QScript::Chunk_t& chunk, int offset );
};
