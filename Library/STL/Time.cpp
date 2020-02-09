#include "QLibPCH.h"
#include "NativeModule.h"
#include "Time.h"

#include "../Common/Chunk.h"
#include "../Compiler/Compiler.h"
#include "../Runtime/QVM.h"

#include <time.h>

QScript::Value Native_Clock( const QScript::Value* args, int numArgs );

TimeModule::TimeModule()
{
	m_Name = "Time";
}

void TimeModule::Import( VM_t* vm ) const
{
	vm->CreateNative( "clock", &Native_Clock );
}

void TimeModule::Import( Compiler::Assembler* assembler ) const
{
	assembler->AddGlobal( "clock", true, QScript::VT_OBJECT, QScript::OT_NATIVE );
}

QScript::Value Native_Clock( const QScript::Value* args, int numArgs )
{
	return MAKE_NUMBER( (double) clock() / CLOCKS_PER_SEC );
}
