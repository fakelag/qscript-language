#include "QLibPCH.h"
#include "NativeModule.h"
#include "Chunk.h"

#include "Compiler.h"
#include "QVM.h"

#include <time.h>

QScript::Value Native_Clock( const QScript::Value* args, int numArgs );

class QTime : QScript::QNativeModule
{
	QTime();

	const std::string& GetName() const { return "Time"; }

	void Import( VM_t* vm ) const;
	void Import( Compiler::Assembler* assembler ) const;
};

QTime::QTime()
{
	QScript::RegisterModule( this );
}

void QTime::Import( VM_t* vm ) const
{
	vm->CreateNative( "clock", &Native_Clock );
}

void QTime::Import( Compiler::Assembler* assembler ) const
{
	assembler->AddGlobal( "clock", true, QScript::VT_OBJECT, QScript::OT_NATIVE );
}

QScript::Value Native_Clock( const QScript::Value* args, int numArgs )
{
	return MAKE_NUMBER( (double) clock() / CLOCKS_PER_SEC );
}

QTime g_Time;
