#include "QLibPCH.h"
#include "Utils.h"

#include "../Library/Common/Chunk.h"
#include "../Library/Common/Value.h"
#include "../Library/Runtime/QVM.h"
#include "../Library/Compiler/Compiler.h"

std::string TestUtils::GenerateSequence( int length, std::function< std::string( int iteration ) > iterFn,
	const std::string& first, const std::string& last )
{
	std::string output = first;
	for ( int i = 0; i < length; ++i )
		output += iterFn( i );

	return output + last;
}

bool TestUtils::RunVM( const std::string& code, QScript::Value* exitCode )
{
	auto chunk = QScript::Compile( code );

	VM_t vm( chunk );
	QScript::Interpret( vm, exitCode );

	bool vmState = CheckVM( vm );

	vm.Release( exitCode );
	QScript::FreeChunk( chunk );

	return vmState;
}

bool TestUtils::CheckVM( VM_t& vm )
{
	if ( vm.m_StackTop != vm.m_Stack )
		return false;

	return true;
}
