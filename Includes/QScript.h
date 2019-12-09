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

namespace QScript
{
	struct Chunk_t;
	struct Value;

	Chunk_t* AllocChunk();
	void FreeChunk( Chunk_t* chunk );

	Chunk_t* Compile( const char* source );
	void Interpret( const Chunk_t& chunk, Value* exitCode );
}
