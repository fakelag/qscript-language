#pragma once
#include "NativeModule.h"

class QTime : public QScript::QNativeModule
{
public:
	QTime();

	const std::string& GetName() const { return m_Name; }

	void Import( VM_t* vm ) const;
	void Import( Compiler::Assembler* assembler ) const;
};
