#include "QLibPCH.h"
#include "NativeModule.h"
#include "Time.h"

#include "../Common/Chunk.h"
#include "../Compiler/Compiler.h"
#include "../Runtime/QVM.h"

#include <time.h>

QScript::Value Native_Clock( void* frame, const QScript::Value* args, int numArgs );

TimeModule::TimeModule()
{
	m_Name = "Time";
}

void TimeModule::Import( VM_t* vm ) const
{
	vm->CreateNative( "clock", &Native_Clock );
}

void TimeModule::Import( Compiler::Assembler* assembler, int lineNr, int colNr ) const
{
	auto& config = assembler->Config();
	assembler->AddGlobal( "clock", true, -1, -1, Compiler::TYPE_NATIVE, Compiler::TYPE_NUMBER );

	if ( config.m_ImportCb )
	{
		std::vector< QScript::NativeFunctionSpec_t > functions ={
			QScript::NativeFunctionSpec_t{ "clock", { }, Compiler::TYPE_NUMBER },
		};

		config.m_ImportCb( lineNr, colNr, m_Name, functions );
	}
}

QScript::Value Native_Clock( void* frame, const QScript::Value* args, int numArgs )
{
	return MAKE_NUMBER( (double) clock() / CLOCKS_PER_SEC );
}
