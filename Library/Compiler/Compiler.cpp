#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Instructions.h"
#include "Compiler.h"

namespace QScript
{
	Function_t* Compile( const std::string& source, int flags )
	{
		Object::AllocateString = &Compiler::AllocateString;
		Object::AllocateFunction = &Compiler::AllocateFunction;
		Object::AllocateNative = &Compiler::AllocateNative;
		Object::AllocateClosure = &Compiler::AllocateClosure;
		Object::AllocateUpvalue = &Compiler::AllocateUpvalue;

		Chunk_t* chunk = AllocChunk();

		// Lexical analysis (tokenization)
		auto tokens = Compiler::Lexer( source );

#if 1
		// Generate IR
		auto entryNodes = Compiler::GenerateIR( tokens );

		// Run IR optimizers

		// Compile bytecode
		Compiler::Assembler assembler( chunk, flags );

		try
		{
			for ( auto node : entryNodes )
				node->Compile( assembler );
		}
		catch ( const CompilerException& exception )
		{
			throw std::vector< CompilerException >{ exception };
		}

		for ( auto node : entryNodes )
		{
			node->Release();
			delete node;
		}

		entryNodes.clear();
#else
		// RET
		chunk->m_Code.push_back( QScript::OpCode::OP_RETURN );
#endif

		// Compiled funtions
		auto functions = assembler.Finish();

		// Clean up objects created in compilation process
		Compiler::GarbageCollect( functions );

		// Reset allocators
		Object::AllocateString = NULL;
		Object::AllocateFunction = NULL;
		Object::AllocateNative = NULL;
		Object::AllocateClosure = NULL;
		Object::AllocateUpvalue = NULL;

		// Return compiled code
		return functions.back();
	}
}

namespace Compiler
{
	// List of allocated objects for garbage collection
	std::vector< QScript::Object* > ObjectList;

	uint32_t AddConstant( const QScript::Value& value, QScript::Chunk_t* chunk )
	{
		chunk->m_Constants.push_back( value );
		return ( uint32_t ) chunk->m_Constants.size() - 1;
	}

	void EmitByte( uint8_t byte, QScript::Chunk_t* chunk )
	{
		chunk->m_Code.push_back( byte );
	}

	QScript::StringObject* AllocateString( const std::string& string )
	{
		// Compiler object allocation must use pure 'new'
		// keyword, since FreeChunk() is also responsible for releasing these objects
		auto stringObject = new QScript::StringObject( string );
		ObjectList.push_back( ( QScript::Object* ) stringObject );
		return stringObject;
	}

	QScript::FunctionObject* AllocateFunction( const std::string& name, int arity )
	{
		auto functionObject = new QScript::FunctionObject( new QScript::Function_t( name, arity ) );
		ObjectList.push_back( ( QScript::Object* ) functionObject );
		return functionObject;
	}

	QScript::NativeFunctionObject* AllocateNative( void* nativeFn )
	{
		auto nativeObject = new QScript::NativeFunctionObject( ( QScript::NativeFn ) nativeFn );
		ObjectList.push_back( ( QScript::Object* ) nativeObject );
		return nativeObject;
	}

	QScript::ClosureObject* AllocateClosure( QScript::FunctionObject* function )
	{
		assert( 0 );

		auto closureObject = new QScript::ClosureObject( function );
		ObjectList.push_back( ( QScript::Object* ) closureObject );
		return closureObject;
	}

	QScript::UpvalueObject* AllocateUpvalue( QScript::Value* valuePtr )
	{
		assert( 0 );

		auto upvalueObject = new QScript::UpvalueObject( valuePtr );
		ObjectList.push_back( ( QScript::Object* ) upvalueObject );
		return upvalueObject;
	}

	void GarbageCollect( const std::vector< QScript::Function_t* >& functions )
	{
		for ( auto object : ObjectList )
		{
			bool isReferenced = false;

			for ( auto function : functions )
			{
				for ( auto value : function->m_Chunk->m_Constants )
				{
					if ( IS_OBJECT( value ) && AS_OBJECT( value ) == object )
						isReferenced = true;
				}

				if ( isReferenced )
					break;
			}

			if ( !isReferenced )
				delete object;
		}

		ObjectList.clear();
	}

	Assembler::Assembler( QScript::Chunk_t* chunk, int optimizationFlags )
	{
		m_OptimizationFlags = optimizationFlags;

		// Main code
		CreateFunction( "<main>", 0, chunk );
	}

	std::vector< QScript::Function_t* > Assembler::Finish()
	{
		delete CurrentStack();
		m_Compiled.push_back( CurrentFunction() );
		m_Functions.pop_back();

		return m_Compiled;
	}

	QScript::Chunk_t* Assembler::CurrentChunk()
	{
		return m_Functions.back().m_Func->m_Chunk;
	}

	QScript::Function_t* Assembler::CurrentFunction()
	{
		return m_Functions.back().m_Func;
	}

	Assembler::Stack_t* Assembler::CurrentStack()
	{
		return m_Functions.back().m_Stack;
	}

	QScript::Function_t* Assembler::CreateFunction( const std::string& name, int arity, QScript::Chunk_t* chunk )
	{
		auto function = new QScript::Function_t( name, arity, chunk );
		auto context = FunctionContext_t{ function, new Assembler::Stack_t() };

		m_Functions.push_back( context );

		CreateLocal( name == "<main>" ? "" : name );
		return function;
	}

	void Assembler::FinishFunction( QScript::FunctionObject** func, std::vector< Upvalue_t >* upvalues )
	{
		auto function = &m_Functions.back();

		*func = new QScript::FunctionObject( function->m_Func );
		*upvalues = function->m_Upvalues;

		// Implicit return (null)
		EmitByte( QScript::OpCode::OP_LOAD_NULL, function->m_Func->m_Chunk );
		EmitByte( QScript::OpCode::OP_RETURN, function->m_Func->m_Chunk );

		// Finished compiling
		m_Compiled.push_back( function->m_Func );

		// Free compile-time stack from memory
		delete CurrentStack();
		m_Functions.pop_back();
	}

	uint32_t Assembler::CreateLocal( const std::string& name )
	{
		auto stack = CurrentStack();

		stack->m_Locals.push_back( Assembler::Local_t{ name, stack->m_CurrentDepth, false } );
		return ( uint32_t ) stack->m_Locals.size() - 1;
	}

	Assembler::Local_t* Assembler::GetLocal( int local )
	{
		return &CurrentStack()->m_Locals[ local ];
	}

	bool Assembler::FindLocalFromStack( Stack_t* stack, const std::string& name, uint32_t* out )
	{
		for ( int i = ( int ) stack->m_Locals.size() - 1; i >= 0 ; --i )
		{
			if ( stack->m_Locals[ i ].m_Name == name )
			{
				*out = ( uint32_t ) i;
				return true;
			}
		}

		return false;
	}

	bool Assembler::FindLocal( const std::string& name, uint32_t* out )
	{
		return FindLocalFromStack( CurrentStack(), name, out );
	}

	bool Assembler::RequestUpvalue( const std::string name, uint32_t* out )
	{
		int thisFunction = ( int ) m_Functions.size() - 1;
		for ( int i = thisFunction; i > 0; --i )
		{
			uint32_t upValue = 0;
			if ( !FindLocalFromStack( m_Functions[ i - 1 ].m_Stack, name, &upValue ) )
				continue;

			// Capture it
			m_Functions[ i - 1 ].m_Stack->m_Locals[ upValue ].m_Captured = true;

			// Link upvalue through the closure chain
			for ( int j = i; j <= thisFunction; ++j )
				upValue = AddUpvalue( &m_Functions[ j ], upValue, j == i );

			*out = upValue;
			return true;
		}

		return false;
	}

	uint32_t Assembler::AddUpvalue( FunctionContext_t* context, uint32_t index, bool isLocal )
	{
		for ( uint32_t i = 0; i < context->m_Upvalues.size(); ++i )
		{
			auto upvalue = context->m_Upvalues[ i ];
			if ( upvalue.m_Index == index && upvalue.m_IsLocal == isLocal )
				return i;
		}

		context->m_Upvalues.push_back( Upvalue_t{ isLocal, index } );
		return ( context->m_Func->m_NumUpvalues = ( int ) context->m_Upvalues.size() ) - 1;
	}

	int Assembler::StackDepth()
	{
		return CurrentStack()->m_CurrentDepth;
	}

	void Assembler::PushScope()
	{
		++CurrentStack()->m_CurrentDepth;
	}

	void Assembler::PopScope()
	{
		auto stack = CurrentStack();

		for ( int i = ( int ) stack->m_Locals.size() - 1; i >= 0; --i )
		{
			auto local = stack->m_Locals[ i ];

			if ( local.m_Depth < stack->m_CurrentDepth )
				break;

			if ( !local.m_Captured )
				EmitByte( QScript::OpCode::OP_POP, CurrentChunk() );
			else
				EmitByte( QScript::OpCode::OP_CLOSE_UPVALUE, CurrentChunk() );

			stack->m_Locals.erase( stack->m_Locals.begin() + i );
		}

		--stack->m_CurrentDepth;
	}

	int Assembler::OptimizationFlags() const
	{
		return m_OptimizationFlags;
	}
}
