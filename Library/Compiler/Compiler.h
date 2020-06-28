#pragma once

#include "Tokens.h"
#include "AST/AST.h"
#include "Typing.h"

struct VM_t;
class Object;

namespace Compiler
{
	struct Type_t;

	std::vector< Token_t > Lexer( const std::string& source );
	std::vector< BaseNode* > GenerateIR( const std::vector< Token_t >& tokens );
	// std::vector< BaseNode* > OptimizeIR( std::vector< BaseNode* > nodes );

	// Bytecode
	uint32_t AddConstant( const QScript::Value& value, QScript::Chunk_t* chunk );
	void EmitByte( uint8_t byte, QScript::Chunk_t* chunk );

	// Object allocation
	QScript::StringObject* AllocateString( const std::string& string );
	QScript::FunctionObject* AllocateFunction( const std::string& name );
	QScript::NativeFunctionObject* AllocateNative( void* nativeFn );
	QScript::ClosureObject* AllocateClosure( QScript::FunctionObject* function );
	QScript::UpvalueObject* AllocateUpvalue( QScript::Value* valuePtr );
	QScript::TableObject* AllocateTable( const std::string& name );
	QScript::ArrayObject* AllocateArray( const std::string& name );

	void GarbageCollect( const std::vector< QScript::FunctionObject* >& functions );
	void GarbageCollect( const std::vector< Compiler::BaseNode* >& nodes );
	void GarbageCollect( const std::vector< QScript::Value >& values );

	struct Variable_t
	{
		Variable_t() : m_Type( TYPE_UNKNOWN )
		{
			m_IsConst = false;
			m_Function = NULL;
		}

		std::string					m_Name;
		bool						m_IsConst;
		Type_t						m_Type;
		QScript::FunctionObject*	m_Function;
	};

	struct Local_t
	{
		Variable_t		m_Var;
		uint32_t		m_Depth;
		bool			m_Captured;
	};

	class Assembler
	{
	public:
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
			Type_t 						m_Type;
			std::vector< Upvalue_t >	m_Upvalues;
		};

		Assembler( QScript::Chunk_t* chunk, const QScript::Config_t& config );

		void 										AddArgument( const std::string& name, bool isConstant, int lineNr, int colNr, Type_t type );
		bool 										AddGlobal( const std::string& name, bool isConstant, int lineNr, int colNr,
																Type_t type, QScript::FunctionObject* fn = NULL );

		uint32_t 									AddLocal( const std::string& name, bool isConstant, int lineNr, int colNr,
																Type_t type, QScript::FunctionObject* fn = NULL );

		uint32_t 									AddUpvalue( FunctionContext_t* context, uint32_t index, bool isLocal, int lineNr, int colNr, Variable_t* varInfo );
		void 										ClearArguments();
		const QScript::Config_t&					Config() const;
		QScript::FunctionObject*					CreateFunction( const std::string& name, bool isConst, Type_t type, bool isAnonymous, bool addLocal, QScript::Chunk_t* chunk );
		const std::vector< Variable_t >& 			CurrentArguments();
		QScript::Chunk_t*							CurrentChunk();
		const FunctionContext_t*					CurrentContext();
		QScript::FunctionObject*					CurrentFunction();
		Stack_t*									CurrentStack();
		bool 										FindArgument( const std::string& name, Variable_t* out );
		bool 										FindGlobal( const std::string& name, Variable_t* out );
		bool										FindLocal( const std::string& name, uint32_t* out, Variable_t* varInfo );
		bool 										FindLocalFromStack( Stack_t* stack, const std::string& name, uint32_t* out, Variable_t* varInfo );
		bool 										FindUpvalue( const std::string name, uint32_t* out, Variable_t* varInfo );
		void										FinishFunction( int lineNr, int colNr, std::vector< Upvalue_t >* upvalues );
		std::vector< QScript::FunctionObject* >		Finish();
		Local_t*									GetLocal( int local );
		bool										IsTopLevel();
		void										PopScope();
		void										PushScope();
		void										Release();
		bool 										RequestUpvalue( const std::string name, uint32_t* out, int lineNr, int colNr, Variable_t* varInfo );
		int											StackDepth();

	private:
		std::vector< Variable_t >						m_FunctionArgs;
		std::vector< FunctionContext_t >				m_Functions;
		QScript::Config_t								m_Config;

		std::vector< QScript::FunctionObject* >			m_Compiled;
		std::map< std::string, Variable_t >				m_Globals;
	};
};
