#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Instructions.h"
#include "Compiler.h"

namespace QScript
{
	Chunk_t* Compile( const char* source )
	{
		Object::AllocateString = &Compiler::AllocateString;

		Chunk_t* chunk = AllocChunk();

		// Lexical analysis (tokenization)
		auto tokens = Compiler::Lexer( std::string( source ) );

#if 1
		// Generate IR
		auto entryNodes = Compiler::GenerateIR( tokens );

		// Run IR optimizers

		// Compile bytecode
		for ( auto node : entryNodes )
			node->Compile( chunk );

		for ( auto node : entryNodes )
			delete node;

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

	uint8_t AddConstant( const QScript::Value& value, QScript::Chunk_t* chunk )
	{
		chunk->m_Constants.push_back( value );
		return ( uint8_t ) chunk->m_Constants.size() - 1;
	}

	void EmitByte( uint8_t byte, QScript::Chunk_t* chunk )
	{
		chunk->m_Code.push_back( byte );
	}

	QScript::StringObject* AllocateString( const std::string& string )
	{
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

			if (!isReferenced)
				delete object;
		}

		ObjectList.clear();
	}
}
