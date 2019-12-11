#include "QLibPCH.h"
#include "Chunk.h"

QScript::Object::StringAllocatorFn QScript::Object::AllocateString = NULL;

QScript::Chunk_t* QScript::AllocChunk()
{
	return new Chunk_t;
}

void QScript::FreeChunk( QScript::Chunk_t* chunk )
{
	delete chunk;
}
