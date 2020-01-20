#pragma once

#if defined(_WIN32)
#ifndef FORCEINLINE
#define FORCEINLINE __forceinline
#endif
#elif defined(_OSX)
#ifndef FORCEINLINE
#define FORCEINLINE inline
#endif
#endif

struct VM_t;

namespace QScript
{
	struct Chunk_t;
	class FunctionObject;
	struct Value;

	enum OptimizationFlags
	{
		OF_NONE = 0,
		OF_CONSTANT_STACKING = ( 1<<0 ),
	};

	Chunk_t* AllocChunk();
	void FreeChunk( Chunk_t* chunk );
	void FreeFunction( FunctionObject* function );

	FunctionObject* Compile( const std::string& source, int flags = OF_NONE );

	void Interpret( const FunctionObject& function );
	void Interpret( VM_t& vm, Value* exitCode );
}
