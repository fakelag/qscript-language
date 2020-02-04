#include "QLibPCH.h"
#include "NativeModule.h"
#include "System.h"

#include "../Common/Chunk.h"
#include "../Compiler/Compiler.h"
#include "../Runtime/QVM.h"

#include <time.h>

QScript::Value Native_Exit( const QScript::Value* args, int numArgs );
QScript::Value Native_Print( const QScript::Value* args, int numArgs );

QSystem::QSystem()
{
	m_Name = "System";
}

void QSystem::Import( VM_t* vm ) const
{
	vm->CreateNative( "exit", &Native_Exit );
	vm->CreateNative( "print", &Native_Print );
}

void QSystem::Import( Compiler::Assembler* assembler ) const
{
	assembler->AddGlobal( "exit", true, QScript::VT_OBJECT, QScript::OT_NATIVE );
	assembler->AddGlobal( "print", true, QScript::VT_OBJECT, QScript::OT_NATIVE );
}

QScript::Value Native_Exit( const QScript::Value* args, int numArgs )
{
	throw RuntimeException( "rt_exit", "exit() called", 0, 0, "" );
}

QScript::Value Native_Print( const QScript::Value* args, int numArgs )
{
	for ( auto arg = args; arg < args + numArgs; ++arg )
		std::cout << arg->ToString();

	std::cout << std::endl;
	return MAKE_NULL;
}
