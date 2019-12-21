#pragma once

#include "Tokens.h"
#include "AST.h"

struct VM_t;
class Object;

namespace Compiler
{
	std::vector< Token_t > Lexer( const std::string& source );
	std::vector< BaseNode* > GenerateIR( const std::vector< Token_t >& tokens );
	// std::vector< BaseNode* > OptimizeIR( std::vector< BaseNode* > nodes );

	// Compilers
	uint32_t AddConstant( const QScript::Value& value, QScript::Chunk_t* chunk );
	void EmitByte( uint8_t byte, QScript::Chunk_t* chunk );

	// Disassembler
	void DisassembleChunk( const QScript::Chunk_t& chunk, const std::string& identifier, unsigned int ip = 0 );
	int DisassembleInstruction( const QScript::Chunk_t& chunk, int offset, bool isIp );

	void DumpConstants( const QScript::Chunk_t& chunk );
	void DumpGlobals( const VM_t& vm );
	void DumpStack( const VM_t& vm );

	std::string ValueToString( const QScript::Value& value );

	// Object allocation
	QScript::StringObject* AllocateString( const std::string& string );
	void GarbageCollect( const QScript::Chunk_t* chunk );
};
