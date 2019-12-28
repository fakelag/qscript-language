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
			var glob3;											\
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
				glob3 = loc2;									\
			}\
			return glob3;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 76.00 );

		// > 255 local variables
		std::string localVariables = TestUtils::GenerateSequence( 500, []( int iter ) {
			return "var tmp_" + std::to_string( iter ) + " = " + std::to_string( iter ) + ";";
		} );

		UTEST_ASSERT( TestUtils::RunVM( "var glob1 = 60;		\
			{ " + localVariables + " glob1 = tmp_412; }			\
			return glob1;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 412 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "If-else clause" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var global = 50;	\
			var res;										\
			if (global == 50)								\
			{												\
				var local = global + 20;					\
				if (local == global)						\
				{											\
					return \"wrong\";						\
				}											\
				else										\
				{											\
					{										\
						var local2 = local + 30;			\
						res = local2;						\
					}										\
				}											\
			}												\
			else											\
			{												\
				return \"wrong\";							\
			}\
			return res;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 100.00 );

		UTEST_ASSERT( TestUtils::RunVM( "var x = 0;								\
			if (x == 1) {														\
				var a = \"hello \";												\
				{																\
					var b = a + \" hello\";										\
					x = b * 2 + 10;												\
					if ( x > 0 ) { x = \"unreachable unreachable\"; }			\
					else { x = \"unreachable unreachable\"; }					\
				}																\
			} else {															\
				var b = 1000;													\
				if ( x == 0 ) {													\
					var f = x + 10;												\
					if ( f == 20 ) { f = f + 1; x = f; }						\
					else { var z = f + 120; x = \"number is \" + (z + f + b); }	\
				} else {														\
					x = 60;														\
					if ( 1 ) { x = 90; }										\
					else { x = 120 * x; }										\
				}																\
			}																	\
		return x;", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "number is 1140." );

		UTEST_ASSERT( TestUtils::RunVM( "var x = 0;								\
			if (x) { x = 1; }													\
			else if (0) { x = 2; }												\
			else if (x == 1) { x = 3; }											\
			else if (x == 0) { x = 4; }											\
			else if (x == 0) { x = 5; }											\
			else { x = 6; }														\
		return x;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 4 );

		// Generate a body of more than 255 instructions
		auto largeBodyOfCode = TestUtils::GenerateSequence( 1024, []( int iter ) {
			return "_tmp = _tmp+" + std::to_string( iter ) + std::string( ".00;" );
		}, "var _tmp = 0;" );

		UTEST_ASSERT( TestUtils::RunVM( "var x = 0;								\
				if (x) { " + largeBodyOfCode + " }								\
				else if (x == 1) { " + largeBodyOfCode + " }					\
				else { " + largeBodyOfCode + " x = 42; }						\
			return x;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 42 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_END();
}
