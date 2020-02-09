#pragma once
#include "NativeModule.h"

class SystemModule : public QScript::NativeModule
{
public:
	SystemModule();

	const std::string& GetName() const { return m_Name; }

	void Import( VM_t* vm ) const;
	void Import( Compiler::Assembler* assembler ) const;
};
