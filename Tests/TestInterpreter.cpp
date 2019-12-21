#include "QLibPCH.h"
#include "Tests.h"

#include "../Library/Common/Chunk.h"
#include "../Library/Compiler/Compiler.h"

#include "Utils.h"

using namespace Tests;
using namespace Compiler;

bool Tests::TestInterpreter()
{
	UTEST_BEGIN( "Interpreter Tests" );

	UTEST_CASE( "Simple number addition" )
	{
		auto chunk = QScript::Compile( "return 2 + 2;" );

		QScript::Value exitCode;
		QScript::Interpret( *chunk, &exitCode );

		UTEST_ASSERT( IS_NUMBER( exitCode ) == true );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 4.0 );

		QScript::FreeChunk( chunk );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "String concatenation" )
	{
		auto chunk = QScript::Compile( "return \"lo\" + \"ng\"+ \" \" + \"str\" + \"ing\";" );

		QScript::Value exitCode;
		QScript::Interpret( *chunk, &exitCode );

		UTEST_ASSERT( IS_STRING( exitCode ) == true );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "long string" );

		QScript::FreeChunk( chunk );
		chunk = QScript::Compile( "return \"string\" + 1;" );

		QScript::Interpret( *chunk, &exitCode );

		UTEST_ASSERT( IS_STRING( exitCode ) == true );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "string1.00" );

		QScript::FreeChunk( chunk );
		chunk = QScript::Compile( "return 2.5 + \"string\";" );

		QScript::Interpret( *chunk, &exitCode );

		UTEST_ASSERT( IS_STRING( exitCode ) == true );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "2.50string" );

		QScript::FreeChunk( chunk );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Adding up constants (index > 255)" )
	{
		auto chunk = QScript::Compile( TestUtils::GenerateSequence( 512, []( int iter ) {
			return std::to_string( iter ) + std::string( ".00" ) + ( ( iter < 511 ) ? "+" : "" );
		}, "return ", ";" ) );

		UTEST_ASSERT( chunk->m_Constants.size() == 512 );

		QScript::Value exitCode;
		QScript::Interpret( *chunk, &exitCode );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 130816.0 );

		QScript::FreeChunk( chunk );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_END();
}
