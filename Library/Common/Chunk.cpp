#include "QLibPCH.h"
#include "Chunk.h"

QScript::Object::StringAllocatorFn QScript::Object::AllocateString = NULL;
QScript::Object::FunctionAllocatorFn QScript::Object::AllocateFunction = NULL;
QScript::Object::NativeAllocatorFn QScript::Object::AllocateNative = NULL;
QScript::Object::ClosureAllocatorFn QScript::Object::AllocateClosure = NULL;
QScript::Object::UpvalueAllocatorFn QScript::Object::AllocateUpvalue = NULL;
QScript::Object::TableAllocatorFn QScript::Object::AllocateTable = NULL;

QScript::Chunk_t* QScript::AllocChunk()
{
	return QS_NEW Chunk_t;
}

void FreeObject( QScript::Object* object )
{
	switch ( object->m_Type )
	{
	case QScript::OT_FUNCTION:
	{
		QScript::FreeChunk( ( ( QScript::FunctionObject* ) object )->GetChunk() );
		break;
	}
	case QScript::OT_CLOSURE:
	{
		assert( 0 );
		break;
	}
	case QScript::OT_UPVALUE:
	{
		assert( 0 );
		break;
	}
	default:
		break;
	}

	delete object;
}

void QScript::FreeChunk( QScript::Chunk_t* chunk )
{
	for ( auto constant : chunk->m_Constants )
	{
		if ( !IS_OBJECT( constant ) )
			continue;

		auto object = AS_OBJECT( constant );
		FreeObject( object );
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
