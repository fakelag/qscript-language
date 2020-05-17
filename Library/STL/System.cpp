#include "QLibPCH.h"
#include "NativeModule.h"
#include "System.h"
#include "Array.h"

#include "../Common/Chunk.h"
#include "../Compiler/Compiler.h"
#include "../Runtime/QVM.h"

#include <time.h>

QScript::Value Native_Exit( void* frame, const QScript::Value* args, int numArgs );
QScript::Value Native_Print( void* frame, const QScript::Value* args, int numArgs );

SystemModule::SystemModule()
{
	m_Name = "System";
}

void SystemModule::Import( VM_t* vm ) const
{
	vm->CreateNative( "exit", &Native_Exit );
	vm->CreateNative( "print", &Native_Print );

	ArrayModule::LoadMethods( vm );
}

void SystemModule::Import( Compiler::Assembler* assembler ) const
{
	assembler->AddGlobal( "exit", true, Compiler::TYPE_NATIVE, Compiler::TYPE_NONE );
	assembler->AddGlobal( "print", true, Compiler::TYPE_NATIVE, Compiler::TYPE_NONE );
}

QScript::Value Native_Exit( void* frame, const QScript::Value* args, int numArgs )
{
	throw RuntimeException( "rt_exit", "exit() called", 0, 0, "" );
}

QScript::Value Native_Print( void* frame, const QScript::Value* args, int numArgs )
{
	for ( auto arg = args + 1; arg < args + numArgs; ++arg )
		std::cout << arg->ToString();

	std::cout << std::endl;
	return MAKE_NULL;
}
