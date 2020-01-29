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
	void DisassembleChunk( const QScript::Chunk_t& chunk, const std::string& identifier, int ip = -1 );
	uint32_t DisassembleInstruction( const QScript::Chunk_t& chunk, uint32_t offset, bool isIp );
	int InstructionSize( uint8_t inst );
	void DumpConstants( const QScript::Chunk_t& chunk );
	void DumpGlobals( const VM_t& vm );
	void DumpStack( const VM_t& vm );
	bool FindDebugSymbol( const QScript::Chunk_t& chunk, uint32_t offset, QScript::Chunk_t::Debug_t* out );

	// Object allocation
	QScript::StringObject* AllocateString( const std::string& string );
	QScript::FunctionObject* AllocateFunction( const std::string& name, int arity );
	QScript::NativeFunctionObject* AllocateNative( void* nativeFn );
	QScript::ClosureObject* AllocateClosure( QScript::FunctionObject* function );
	QScript::UpvalueObject* AllocateUpvalue( QScript::Value* valuePtr );
	void GarbageCollect( const std::vector< QScript::FunctionObject* >& functions );

	class Assembler
	{
	public:
		struct Variable_t
		{
			std::string				m_Name;
			bool					m_IsConst;
			QScript::ValueType		m_Type;
			QScript::ObjectType		m_ObjType;
		};

		struct Local_t
		{
			Variable_t		m_Var;
			uint32_t		m_Depth;
			bool			m_Captured;
		};

		struct Stack_t
		{
			Stack_t()
			{
				m_CurrentDepth = 0;
			}

			std::vector< Local_t >	m_Locals;
			uint32_t				m_CurrentDepth;
		};

		struct Upvalue_t
		{
			bool 					m_IsLocal;
			uint32_t 				m_Index;
		};

		struct FunctionContext_t
		{
			QScript::FunctionObject*	m_Func;
			Stack_t*					m_Stack;
			std::vector< Upvalue_t >	m_Upvalues;
		};

		Assembler( QScript::Chunk_t* chunk, const QScript::Config_t& config );

		bool 										AddGlobal( const std::string& name );
		bool 										AddGlobal( const std::string& name, bool isConstant, QScript::ValueType type, QScript::ObjectType objType );
		uint32_t 									AddUpvalue( FunctionContext_t* context, uint32_t index, bool isLocal );
		QScript::FunctionObject*					CreateFunction( const std::string& name, int arity, bool isAnonymous, QScript::Chunk_t* chunk );
		uint32_t 									CreateLocal( const std::string& name, bool isConstant, QScript::ValueType type, QScript::ObjectType objType );
		uint32_t									CreateLocal( const std::string& name );
		QScript::Chunk_t*							CurrentChunk();
		QScript::FunctionObject*					CurrentFunction();
		Stack_t*									CurrentStack();
		bool 										FindGlobal( const std::string& name, Variable_t* out );
		bool										FindLocal( const std::string& name, uint32_t* out, Variable_t* varInfo );
		bool 										FindLocalFromStack( Stack_t* stack, const std::string& name, uint32_t* out, Variable_t* varInfo );
		void										FinishFunction( QScript::FunctionObject** func, std::vector< Upvalue_t >* upvalues );
		std::vector< QScript::FunctionObject* >		Finish();
		Local_t*									GetLocal( int local );
		void										PopScope();
		void										PushScope();
		bool 										RequestUpvalue( const std::string name, uint32_t* out, Variable_t* varInfo );
		int											StackDepth();
		int											OptimizationFlags() const;

	private:
		std::vector< FunctionContext_t >				m_Functions;
		int												m_OptimizationFlags;

		std::vector< QScript::FunctionObject* >			m_Compiled;
		std::map< std::string, Variable_t >				m_Globals;
	};
};
