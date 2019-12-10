#include "QLibPCH.h"
#include "Tests.h"

#include "../Library/Common/Chunk.h"
#include "../Library/Compiler/Compiler.h"

using namespace Tests;
using namespace Compiler;

bool Tests::TestInterpreter()
{
	UTEST_BEGIN( "Interpreter Tests" );

	UTEST_CASE( "Running addition" )
	{
		auto chunk = QScript::Compile( "2 + 2;" );

		QScript::Value exitCode;
		QScript::Interpret( *chunk, &exitCode );

		UTEST_ASSERT( IS_NUMBER( exitCode ) == true );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 4.0 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_END();
}
