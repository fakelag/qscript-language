#pragma once

namespace QScript
{
	struct Chunk_t;
	struct Value;

	Chunk_t* AllocChunk();
	void FreeChunk( Chunk_t* chunk );

	Chunk_t* Compile( const char* source );
	void Interpret( const Chunk_t& chunk );
}
