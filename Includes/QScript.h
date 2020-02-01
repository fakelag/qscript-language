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

	struct Config_t
	{
		enum OptimizationFlags : uint8_t
		{
			OF_NONE = 0,
		};

		Config_t()
		{
			m_OptFlags = OF_NONE;
		}

		uint8_t							m_OptFlags;
		std::vector< std::string >		m_Globals;
	};

	Chunk_t* AllocChunk();
	FunctionObject* Compile( const std::string& source, const Config_t& config = Config_t() );

	void FreeChunk( Chunk_t* chunk );
	void FreeFunction( FunctionObject* function );

	void Repl();
	void Interpret( const FunctionObject& function );
	void Interpret( VM_t& vm, Value* exitCode );
}
