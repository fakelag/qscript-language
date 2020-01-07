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

	void GarbageCollect( const std::vector< QScript::Function_t* >& functions )
	{
		for ( auto object : ObjectList )
		{
			bool isReferenced = false;

			for ( auto function : functions )
			{
				for ( auto value : function->m_Chunk->m_Constants )
				{
					if ( IS_OBJECT( value ) && value.m_Data.m_Object == object )
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
		CreateLocal( "" );
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
		return m_Functions.back().first->m_Chunk;
	}

	QScript::Function_t* Assembler::CurrentFunction()
	{
		return m_Functions.back().first;
	}

	Assembler::Stack_t* Assembler::CurrentStack()
	{
		return m_Functions.back().second;
	}

	QScript::Function_t* Assembler::CreateFunction( const std::string& name, int arity, QScript::Chunk_t* chunk )
	{
		auto function = new QScript::Function_t( name, 0, chunk );

		m_Functions.push_back( { function, new Assembler::Stack_t() } );

		CreateLocal( name );
		return function;
	}

	QScript::FunctionObject* Assembler::FinishFunction()
	{
		auto finishingFunction = CurrentFunction();

		// Release scope
		PopScope();
		EmitByte( QScript::OpCode::OP_RETURN, finishingFunction->m_Chunk );

		// Finished compiling
		m_Compiled.push_back( finishingFunction );

		// Free compile-time stack from memory
		delete CurrentStack();
		m_Functions.pop_back();

		// Return function object
		return new QScript::FunctionObject( finishingFunction );
	}

	uint32_t Assembler::CreateLocal( const std::string& name )
	{
		auto stack = CurrentStack();

		stack->m_Locals.push_back( Assembler::Local_t{ name, stack->m_CurrentDepth } );
		return ( uint32_t ) stack->m_Locals.size() - 1;
	}

	Assembler::Local_t* Assembler::GetLocal( int local )
	{
		return &CurrentStack()->m_Locals[ local ];
	}

	bool Assembler::FindLocal( const std::string& name, uint32_t* out )
	{
		auto stack = CurrentStack();

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

		for ( int i = LocalsInCurrentScope() - 1; i >= 0; --i )
			EmitByte( QScript::OpCode::OP_POP, CurrentChunk() );

		for ( int i = ( int ) stack->m_Locals.size() - 1; i >= 0; --i )
		{
			if ( stack->m_Locals[ i ].m_Depth < stack->m_CurrentDepth )
				break;

			stack->m_Locals.erase( stack->m_Locals.begin() + i );
		}

		--stack->m_CurrentDepth;
	}

	int Assembler::LocalsInCurrentScope()
	{
		auto stack = CurrentStack();

		int count = 0;
		for ( int i = ( int ) stack->m_Locals.size() - 1; i >= 0; --i )
		{
			if ( stack->m_Locals[ i ].m_Depth < stack->m_CurrentDepth )
				return count;

			++count;
		}

		return stack->m_Locals.size();
	}

	int Assembler::OptimizationFlags() const
	{
		return m_OptimizationFlags;
	}
}
