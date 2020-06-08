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

namespace Compiler
{
	class BaseNode;
	struct Variable_t;

	std::string TypeToString( uint32_t type );
	bool TypeCheck( uint32_t targetType, uint32_t exprType, bool strict = true );
}

namespace QScript
{
	struct Chunk_t;
	class FunctionObject;
	struct Value;
	struct NativeFunctionSpec_t;

	using IdentifierCreatedFn = void( *)( int lineNr, int colNr, Compiler::Variable_t& variable, const std::string& context );
	using ImportCreatedFn = void( *)( int lineNr, int colNr, const std::string& moduleName, const std::vector< NativeFunctionSpec_t >& functions );

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
			m_IdentifierCb = NULL;
			m_ImportCb = NULL;
		}

		Config_t( const Config_t& other )
		{
			m_CompilerFlags = other.m_CompilerFlags;
			m_Globals = other.m_Globals;
			m_DebugSymbols = other.m_DebugSymbols;
			m_IdentifierCb = other.m_IdentifierCb;
			m_ImportCb = other.m_ImportCb;
		}

		uint8_t							m_CompilerFlags;
		std::vector< std::string >		m_Globals;
		bool							m_DebugSymbols;
		IdentifierCreatedFn				m_IdentifierCb;
		ImportCreatedFn					m_ImportCb;
	};

	Chunk_t* AllocChunk();
	FunctionObject* Compile( const std::string& source, const Config_t& config = Config_t( true ) );
	std::vector< std::pair< uint32_t, uint32_t > > Typer( const std::string& source, const Config_t& config = Config_t( false ) );
	std::vector< Compiler::BaseNode* > GenerateAST( const std::string& source );

	void FreeChunk( Chunk_t* chunk );
	void FreeFunction( FunctionObject* function );

	void Repl();
	void Interpret( const FunctionObject& function );
	void Interpret( VM_t& vm, Value* exitCode );
}
