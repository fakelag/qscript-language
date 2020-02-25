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

		Config_t( bool debugSymbols )
		{
			m_CompilerFlags = OF_NONE;
			m_DebugSymbols = debugSymbols;
		}

		Config_t( const Config_t& other )
		{
			m_CompilerFlags = other.m_CompilerFlags;
			m_Globals = other.m_Globals;
			m_DebugSymbols = other.m_DebugSymbols;
		}

		uint8_t							m_CompilerFlags;
		std::vector< std::string >		m_Globals;
		bool							m_DebugSymbols;
	};

	Chunk_t* AllocChunk();
	FunctionObject* Compile( const std::string& source, const Config_t& config = Config_t( true ) );
	std::vector< std::pair< uint32_t, uint32_t > > Typer( const std::string& source, const Config_t& config = Config_t( false ) );

	void FreeChunk( Chunk_t* chunk );
	void FreeFunction( FunctionObject* function );

	void Repl();
	void Interpret( const FunctionObject& function );
	void Interpret( VM_t& vm, Value* exitCode );
}

namespace Compiler
{
	std::string TypeToString( uint32_t type );
	bool TypeCheck( uint32_t targetType, uint32_t exprType, bool strict = true );
}
