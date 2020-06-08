#pragma once
#include "NativeModule.h"

class TimeModule : public QScript::NativeModule
{
public:
	TimeModule();

	const std::string& GetName() const { return m_Name; }

	void Import( VM_t* vm ) const;
	void Import( Compiler::Assembler* assembler, int lineNr, int colNr ) const;
};
