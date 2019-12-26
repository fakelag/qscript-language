#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Instructions.h"
#include "Compiler.h"

namespace QScript
{
	Chunk_t* Compile( const std::string& source, int flags )
	{
		Object::AllocateString = &Compiler::AllocateString;

		Chunk_t* chunk = AllocChunk();

		// Lexical analysis (tokenization)
		auto tokens = Compiler::Lexer( source );

#if 1
		// Generate IR
		auto entryNodes = Compiler::GenerateIR( tokens );

		// Run IR optimizers

		// Compile bytecode
		Compiler::Assembler assembler( chunk, flags );
		for ( auto node : entryNodes )
			node->Compile( assembler );

		for ( auto node : entryNodes )
		{
			node->Release();
			delete node;
		}

		entryNodes.clear();
#else
		// RET
		chunk->m_Code.push_back( QScript::OpCode::OP_RETN );
#endif

		// Clean up objects created in compilation process
		Compiler::GarbageCollect( chunk );

		// Reset allocators
		Object::AllocateString = NULL;

		// Return compiled code
		return chunk;
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

	void GarbageCollect( const QScript::Chunk_t* chunk )
	{
		for ( auto object : ObjectList )
		{
			bool isReferenced = false;
			for ( auto value : chunk->m_Constants )
			{
				if ( IS_OBJECT( value ) && value.m_Data.m_Object == object )
					isReferenced = true;
			}

			if ( !isReferenced )
				delete object;
		}

		ObjectList.clear();
	}

	Assembler::Assembler( QScript::Chunk_t* chunk, int optimizationFlags )
	{
		m_Chunk = chunk;
		m_Stack.m_CurrentDepth = 0;
		m_OptimizationFlags = optimizationFlags;
	}

	QScript::Chunk_t* Assembler::CurrentChunk()
	{
		return m_Chunk;
	}

	uint32_t Assembler::CreateLocal( const std::string& name )
	{
		m_Stack.m_Locals.push_back( Assembler::Local_t{ name, m_Stack.m_CurrentDepth } );
		return ( uint32_t ) m_Stack.m_Locals.size() - 1;
	}

	Assembler::Local_t* Assembler::GetLocal( int local )
	{
		return &m_Stack.m_Locals[ local ];
	}

	bool Assembler::FindLocal( const std::string& name, uint32_t* out )
	{
		for ( int i = ( int ) m_Stack.m_Locals.size() - 1; i >= 0 ; --i )
		{
			if ( m_Stack.m_Locals[ i ].m_Name == name )
			{
				*out = ( uint32_t ) i;
				return true;
			}
		}

		return false;
	}

	int Assembler::StackDepth()
	{
		return m_Stack.m_CurrentDepth;
	}

	void Assembler::PushScope()
	{
		++m_Stack.m_CurrentDepth;
	}

	void Assembler::PopScope()
	{
		for ( int i = ( int ) m_Stack.m_Locals.size() - 1; i >= 0; --i )
		{
			if ( m_Stack.m_Locals[ i ].m_Depth < m_Stack.m_CurrentDepth )
				break;

			m_Stack.m_Locals.erase( m_Stack.m_Locals.begin() + i );
		}

		--m_Stack.m_CurrentDepth;
	}
	
	int Assembler::LocalCount()
	{
		int count = 0;
		for ( int i = ( int ) m_Stack.m_Locals.size() - 1; i >= 0; --i )
		{
			if ( m_Stack.m_Locals[ i ].m_Depth < m_Stack.m_CurrentDepth )
				return count;

			++count;
		}

		return m_Stack.m_Locals.size();
	}

	int Assembler::OptimizationFlags() const
	{
		return m_OptimizationFlags;
	}
}
