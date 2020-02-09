#pragma once

struct VM_t;

namespace Compiler
{
	class Assembler;
}

namespace QScript
{
	class NativeModule
	{
	public:
		virtual ~NativeModule() {};
		virtual const std::string& GetName() const = 0;

		virtual void Import( VM_t* vm ) const = 0;
		virtual void Import( Compiler::Assembler* assembler ) const = 0;

	protected:
		std::string m_Name;
	};

	const NativeModule* ResolveModule( const std::string& name );
	void InitModules();
}
