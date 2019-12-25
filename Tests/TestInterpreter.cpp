#include "QLibPCH.h"
#include "Tests.h"

#include "../Library/Common/Chunk.h"
#include "../Library/Runtime/QVM.h"

#include "Utils.h"

using namespace Tests;

bool Tests::TestInterpreter()
{
	UTEST_BEGIN( "Interpreter Tests" );

	UTEST_CASE( "Simple number addition" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "return 2 + 2;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) == true );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 4.0 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "String concatenation" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "return \"lo\" + \"ng\"+ \" \" + \"str\" + \"ing\";", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) == true );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "long string" );

		UTEST_ASSERT( TestUtils::RunVM( "return \"string\" + 1;", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) == true );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "string1.00" );

		UTEST_ASSERT( TestUtils::RunVM( "return 2.5 + \"string\";", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) == true );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "2.50string" );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Adding up constants (index > 255)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( TestUtils::GenerateSequence( 512, []( int iter ) {
			return std::to_string( iter ) + std::string( ".00" ) + ( ( iter < 511 ) ? "+" : "" );
		}, "return ", ";" ), &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 130816.0 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Assigning global variables" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var h = \"hello \"; var w; w = \"world!\"; var hw = h + w; return hw;", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "hello world!" );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Local variables" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var glob = 10; { var loc = 42; var loc2 = 10 + loc; glob = loc2 * 2; } return glob;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 104.00 );

		UTEST_ASSERT( TestUtils::RunVM( "var glob = 10; { var loc; loc = 2.5; var loc2 = 10 + loc; glob = loc2 * 2; } return glob;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 25.00 );

		UTEST_ASSERT( TestUtils::RunVM( "var g = 5; g = 10; { var z = g + 1; var x; x = z + 2; g = x * 2; } return g;", &exitCode ) );
		
		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 26.00 );

		UTEST_ASSERT( TestUtils::RunVM( "var glob1 = 60;		\
			var glob2 = 40;										\
			{													\
				var loc1 = glob1;								\
				var loc2;										\
				{												\
					var loc3;									\
					var loc4 = loc1 + glob2;					\
					loc2 = loc4;								\
					loc3 = loc2 / 2.0; /* loc3 is 50 now */		\
					{											\
						var loc5;								\
						loc5 = 1.5;								\
						loc2 = loc3 * loc5; /* loc2 is 75.00*/	\
					}											\
					loc2 = loc2 + 1;							\
				}												\
				return loc2;									\
			}", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 76.00 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_END();
}
