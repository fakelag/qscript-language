#pragma once

struct VM_t;

namespace Compiler
{
	class Assembler;
}

namespace QScript
{
	struct NativeFunctionSpec_t
	{
		struct NativeArg_t
		{
			std::string			m_ArgName;
			uint32_t			m_TypeBits;
			uint32_t			m_ReturnTypeBits;
			bool				m_IsVarArgs;
		};

		std::string					m_Name;
		std::vector< NativeArg_t >	m_Args;
		uint32_t					m_ReturnTypeBits;
	};

	class NativeModule
	{
	public:
		virtual ~NativeModule() {};
		virtual const std::string& GetName() const = 0;

		virtual void Import( VM_t* vm ) const = 0;
		virtual void Import( Compiler::Assembler* assembler, int lineNr = -1, int colNr = -1 ) const = 0;

	protected:
		std::string m_Name;
	};

	const NativeModule* ResolveModule( const std::string& name );
	void InitModules();
}
