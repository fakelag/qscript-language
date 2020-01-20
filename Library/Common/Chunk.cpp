#include "QLibPCH.h"
#include "Chunk.h"

QScript::Object::StringAllocatorFn QScript::Object::AllocateString = NULL;
QScript::Object::FunctionAllocatorFn QScript::Object::AllocateFunction = NULL;
QScript::Object::NativeAllocatorFn QScript::Object::AllocateNative = NULL;
QScript::Object::ClosureAllocatorFn QScript::Object::AllocateClosure = NULL;
QScript::Object::UpvalueAllocatorFn QScript::Object::AllocateUpvalue = NULL;

QScript::Chunk_t* QScript::AllocChunk()
{
	return new Chunk_t;
}

void QScript::FreeChunk( QScript::Chunk_t* chunk )
{
	for ( auto constant : chunk->m_Constants )
	{
		if ( !IS_OBJECT( constant ) )
			continue;

		delete AS_OBJECT( constant );
	}

	delete chunk;
}

void QScript::FreeFunction( QScript::FunctionObject* function )
{
	auto chunk = function->GetChunk();

	if ( chunk )
		FreeChunk( chunk );

	delete function;
}
