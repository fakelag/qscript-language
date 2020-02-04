#pragma once
#include "NativeModule.h"

class QSystem : public QScript::QNativeModule
{
public:
	QSystem();

	const std::string& GetName() const { return m_Name; }

	void Import( VM_t* vm ) const;
	void Import( Compiler::Assembler* assembler ) const;
};
