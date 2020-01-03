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

	// Bytecode
	uint32_t AddConstant( const QScript::Value& value, QScript::Chunk_t* chunk );
	void EmitByte( uint8_t byte, QScript::Chunk_t* chunk );

	// Disassembler
	void DisassembleChunk( const QScript::Chunk_t& chunk, const std::string& identifier, unsigned int ip = 0 );
	int DisassembleInstruction( const QScript::Chunk_t& chunk, uint32_t offset, bool isIp );
	int InstructionSize( uint8_t inst );
	void DumpConstants( const QScript::Chunk_t& chunk );
	void DumpGlobals( const VM_t& vm );
	void DumpStack( const VM_t& vm );
	std::string ValueToString( const QScript::Value& value );

	// Object allocation
	QScript::StringObject* AllocateString( const std::string& string );
	QScript::FunctionObject* AllocateFunction( const std::string& name, int arity );
	void GarbageCollect( const QScript::Chunk_t* chunk );

	class Assembler
	{
	public:
		struct Local_t
		{
			std::string		m_Name;
			uint32_t		m_Depth;
		};

		Assembler( QScript::Chunk_t* chunk, int optimizationFlags );

		QScript::Chunk_t*	CurrentChunk();
		Local_t*			GetLocal( int local );
		uint32_t			CreateLocal( const std::string& name );
		bool				FindLocal( const std::string& name, uint32_t* out );
		int					StackDepth();
		int					LocalsInCurrentScope();
		void				PushScope();
		void				PopScope();

		int					OptimizationFlags() const;

	private:
		struct Stack_t
		{
			std::vector< Local_t >	m_Locals;
			uint32_t				m_CurrentDepth;
		};

		Stack_t				m_Stack;
		QScript::Chunk_t*	m_Chunk;
		int					m_OptimizationFlags;
	};
};
