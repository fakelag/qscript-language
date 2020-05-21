#include "QLibPCH.h"
#include "Tests.h"

#include "../Library/Common/Chunk.h"
#include "../Library/Runtime/QVM.h"

#include "Utils.h"

using namespace Tests;

bool Tests::TestInterpreter()
{
#ifdef _DEBUG
	static const int s_LargeConstCount = 10;
#else
	static const int s_LargeConstCount = 300;
#endif

	// Generate a body of more than 255 instructions
	auto largeBodyOfCode = TestUtils::GenerateSequence( s_LargeConstCount, []( int iter ) {
		return "_tmp = _tmp+" + std::to_string( iter ) + std::string( ".00;" );
	}, "var _tmp = 0;" );

	auto largeExpression = TestUtils::GenerateSequence( s_LargeConstCount, []( int iter ) {
		return "0.00" + std::string( iter == ( s_LargeConstCount - 1 ) ? "" : "+" );
	} );

	UTEST_BEGIN( "Interpreter Tests" );

	UTEST_CASE( "Simple number addition" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "return 2 + 2;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) == true );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 4.0 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "String concatenation" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "return \"lo\" + \"ng\"+ \" \" + \"str\" + \"ing\";", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) == true );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "long string" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "return \"string\" + 1;", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) == true );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "string1.00" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "return 2.5 + \"string\";", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) == true );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "2.50string" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Adding up constants (index > 255)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( TestUtils::GenerateSequence( s_LargeConstCount, []( int iter ) {
			return std::to_string( iter ) + std::string( ".00" ) + ( ( iter < s_LargeConstCount - 1 ) ? "+" : "" );
		}, "return ", ";" ), &exitCode ) );

		std::vector< int > sum( s_LargeConstCount );
		std::iota( sum.begin(), sum.end(), 0 );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == std::accumulate( sum.begin(), sum.end(), 0 ) );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Assigning global variables" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var h = \"hello \"; var w; w = \"world!\"; var hw = h + w; return hw;", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "hello world!" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Assigning chaining" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var a = 41; var b; var c; var d; a = b = c = d = a + 1; return a + b + c + d;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 168.0 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Local variables" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var glob = 10; { var loc = 42; var loc2 = 10 + loc; glob = loc2 * 2; } return glob;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 104.00 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var glob = 10; { var loc; loc = 2.5; var loc2 = 10 + loc; glob = loc2 * 2; } return glob;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 25.00 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var g = 5; g = 10; { var z = g + 1; var x; x = z + 2; g = x * 2; } return g;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 26.00 );

		TestUtils::FreeExitCode( exitCode );
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
		std::string localVariables = TestUtils::GenerateSequence( s_LargeConstCount, []( int iter ) {
			return "var tmp_" + std::to_string( iter ) + " = " + std::to_string( iter ) + ";";
		} );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var glob1 = 60;												\
			{ " + localVariables + " glob1 = tmp_" + std::to_string( s_LargeConstCount - 1 ) + "; }		\
			return glob1;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == ( int64_t ) s_LargeConstCount - 1 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE_1( "If-else clause", &largeBodyOfCode )
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

		TestUtils::FreeExitCode( exitCode );
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
		UTEST_ASSERT( AS_STRING( exitCode )->GetString().compare( 0, 15, "number is 1140." ) == 0 );

		TestUtils::FreeExitCode( exitCode );
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

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var x = 0;								\
				if (x) { " + largeBodyOfCode + " }								\
				else if (x == 1) { " + largeBodyOfCode + " }					\
				else { " + largeBodyOfCode + " x = 42; }						\
			return x;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 42 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "And / Or operators" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g1 = 1; var g2;			\
			{															\
				var l2 = g1 || g1 + 100;								\
				var l3 = g1 && g1 + 200;								\
				var l4 = (l2 || l3) && l2;								\
				if (l4 == 1) l2 = 50;									\
				g2 = l2 + l3;											\
			}															\
			return g2;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 251 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "while clause" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g1 = 4;			\
			var g2 = 8;											\
			var g3 = 100000;									\
			while ( g1 < g3 ) {									\
				var l1 = g1;									\
				{												\
					var l2 = l1 + 1 * 2;						\
					var l3 = l2 - 1;							\
					{											\
						var l4 = l3 + 1;						\
						l3 = l4;								\
					}											\
					g1 = l3;									\
				}												\
			}													\
			return g1;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 100000 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var g1 = 0;			\
				while ((g1 = g1 + 1) < 40) {}						\
			return g1;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 40.0 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "do-while clause" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var f = 100; do { f = f - 1; } while (f > 50); return f;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 50 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var g = 0;		\
			var g1 = 0;									\
			while ( g < 10 ) {							\
				var l0 = 0;								\
				var l1 = 25;							\
				do {									\
					l0 = l0 + l1;						\
				} while(l0<100);						\
				{										\
					var l2 = l0;						\
					do {								\
						l2 = l2 * 2;					\
					} while ( l2 < 150 ); 				\
					var l3 = l2 + 50;					\
					g1 = g1 + l3;						\
					g = g + 1;							\
				}										\
			}											\
			return g1 - g; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 2490.0 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var g0 = -1;		\
			var g1 = -1;									\
			do {											\
				g0 = 0;										\
				g1 = 1;										\
			} while ( g0 == -1 || g1 == -1 );				\
			{												\
				var iter = 0;								\
				do {										\
					var sum = g0 + g1;						\
					g0 = g1;								\
					g1 = sum;								\
				} while( ( iter = iter + 1 ) < 50 );		\
			}												\
			return g1; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 20365011074.0 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE_2( "for clause", &largeExpression, &largeBodyOfCode )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g = 0;		\
			for ( var i = 1; i <= 10; i = i + 1 )		\
				g = g + i;								\
		return g; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 55 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var res;				\
			var g0 = 2;											\
			var g1 = 2;											\
			{													\
				var l0 = 10;									\
				for ( ; g0 != 0;) {								\
					g0 = 0;										\
				}												\
				for ( var i = 0; 1 && i < l0; i = i + 1 ) {		\
					g0 = g0 + 1;								\
				}												\
				res = g0;										\
			}													\
			return res; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 10 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var g = 1;																			\
			for ( var i = " + largeExpression + "; i < 10 +" + largeExpression + "; i = i + 1 + " + largeExpression	+ ") {	\
				" + largeBodyOfCode + " g = i + 1;																			\
			} \
			return g;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 10 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE_1( "functions 1 (Simple calls, large functions, args, return values)", &largeBodyOfCode )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var x = 5;				\
			var abc = () -> {									\
				var x = 5;										\
				var y = x + 50;									\
				{ var z = y + y; }								\
				var q = x;										\
			}													\
			[abc];												\
			return 1;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 1 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var x = 5;				\
			var abc = (x) -> {									\
				var y = x + 50;									\
				{ " + largeBodyOfCode + " }						\
				var q = y + x;									\
				return q;										\
			}													\
			" + largeBodyOfCode + "								\
			x = [abc 5];										\
			return x;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 60 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var x = 5;				\
			var abc = (a, b, c) -> {							\
				return a + b + c;								\
			}													\
			return [abc 7, 9, 2];", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 18 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "functions 2 (Calling functions bound to variables, recursive functions, chaining calls)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var x = 0;				\
			var a = (x) -> { var y = x + 5; return y + 5; }		\
			var b = (x) -> { return [a x] + 5; }				\
			var c = (x) -> { return [b x] - 5; }				\
			x = [c 20];											\
			return x;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 30 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "						\
			var fibonacci = (n) -> {							\
				if (n <= 1) return 1;							\
				return [fibonacci n - 1] + [fibonacci n - 2];	\
			}													\
			return [fibonacci 10];", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 89 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var x = 0;							\
			var xxx = (anotherFunc) -> { return [anotherFunc 60, 60]; }		\
			var sum = (a, b) -> { return a + b; }							\
			return [xxx sum];", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 120 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var x = () -> { return () -> { return () -> { return 8; }; } } return [[[x]]];", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 8 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Closures 1 (Simple closures)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var f = () -> {		\
			var l0 = \"initial\";								\
			var a = ( ) -> {									\
				l0 = \"second\";								\
			}													\
			[a];												\
			return l0;											\
		}														\
		return [f];", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "second" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var f = () -> {		\
			var l0 = \"returned\";								\
			var a = ( ) -> {									\
				return l0;										\
			}													\
			return [a];											\
		}														\
		return [f];", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "returned" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var g = () -> {		\
			var a = \"a\";										\
			var b = \"b\";										\
			var f = ( ) -> {									\
				return a + b;									\
			}													\
			return [f];											\
		}														\
		return [g];", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "ab" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var g = null;		\
		{													\
			var l0 = 333;									\
			var l1 = 919191;								\
			var l2 = 222;									\
			var l3 = ( ) -> {								\
				return l1;									\
			}												\
			g = l3;											\
		}													\
		return [g]; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 919191 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Closures 2 (Nested closures, reuse stack slot, shadowing)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g = null;		\
			var a = ( )  -> {								\
			var mmmm;										\
			var l0 = 1;										\
			var bbbb;										\
			var b = ( ) -> {								\
				var oooo;									\
				var l1 = 2;									\
				var ffff;									\
				var c = ( ) -> {							\
					var xxx;								\
					var l2 = 3;								\
					var zzz;								\
					var ggg;								\
					var d = ( ) -> {						\
						g = l0 + l1 + l2;					\
						g = g + l0;							\
					}										\
					[d];									\
				}											\
				[c];										\
			}												\
			[b];											\
		}													\
		[a];												\
		return g; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 7 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var g = null;			\
			var g1 = null;										\
			{													\
				var l0 = null;									\
				{												\
					var a = 1;									\
					var retA = ( ) -> { return a; }				\
					g = retA;									\
				}												\
				{												\
					var b = 2;									\
					g1 = [g];									\
				}												\
			} return g1;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 1 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var g = \"\";			\
			var process = ( ) -> {							\
			var l0 = \"closure\";								\
			var l1 = ( ) -> {									\
				g = g + l0;										\
				var l0 = \"shadow\";							\
				g = g + l0;										\
			}													\
			[l1];												\
			g = g + l0;											\
		}														\
		[process];												\
		return g; ", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "closureshadowclosure" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Closures 3 (Unused closure, close over function param, refer to same variable on all closures)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g = () -> {		\
				var a = \"object\";								\
				if ( false ) { var x = () -> { return a; } }	\
			}													\
			[g];												\
			return 6; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 6 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var g = () -> {		\
				var a = \"object\";								\
				if ( false ) { var x = () -> { return a; } }	\
			}													\
			[g];												\
			return 6; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 6 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var g;							\
			var g2;														\
			{															\
				var a = \"a\";											\
				{														\
					var b = \"b\";										\
					var x = ( ) -> { return a; }						\
					g = x;												\
					if ( false ) { var z = ( ) -> { return b; } }		\
				}														\
				g2 = [g];												\
			}															\
			return g2; ", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "a" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var g = null;					\
			var f = ( arg0 ) -> {										\
				var x = ( ) -> {										\
					return arg0;										\
				}														\
				g = x;													\
			}															\
			[f \"hello\"];												\
			return [g]; ", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "hello" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_ASSERT( TestUtils::RunVM( "var gAdd;					\
			var gGet;												\
			var g = ( ) -> {										\
				var v = 0;											\
				var add = ( ) -> { v = v + 1; }						\
				var get = ( ) -> { return v; }						\
				gAdd = add;											\
				gGet = get;											\
			}														\
			[g];													\
			[gAdd];													\
			[gAdd];													\
			return [gGet]; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 2 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Additional operators (%, **, +=, -=, /=, *=, %=, ++, --)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g1 = 5;							\
			if (1 % 3 != 1 || 3 % 3 != 0) return false;							\
			if (2**2 != 4 || 10**2 != 100) return false;						\
			if ((g1 += 5) != 10 || g1 != 10) return false;						\
			if ((g1 -= 5) != 5 || g1 != 5) return false;						\
			if ((g1 *= 2) != 10 || g1 != 10) return false;						\
			if ((g1 /= 2) != 5 || g1 != 5) return false;						\
			if ((g1 %= 2) != 1 || g1 != 1) return false;						\
			if (++g1 != 2 || g1 != 2) return false;								\
			if (g1++ != 2 || g1 != 3) return false;								\
			if (--g1 != 2 || g1 != 2) return false;								\
			if (g1-- != 2 || g1 != 1) return false;								\
			{ ++g1; --g1; g1--; g1++; }											\
			++g1; --g1; g1--; g1++;												\
			return true;", &exitCode ) );

		UTEST_ASSERT( IS_BOOL( exitCode ) );
		UTEST_ASSERT( AS_BOOL( exitCode ) == true );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Tables (Simple tables)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "Table kova = {			\
				const x = 4;									\
				const y = \"hello\";							\
			};													\
			Table kava;											\
			kava.x = 7;											\
			kava.y = \"bonjour\";								\
			return kova.y + kava.x + kava.y + kova.x;", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "hello7.00bonjour4.00" );

		TestUtils::FreeExitCode( exitCode );

		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0; {				\
				var l0 = 1;											\
				var l1 = 1;											\
				var l2 = 1;											\
				Table kova = {										\
					const x = 4;									\
					const y = \"hello\";							\
				};													\
				{													\
					Table kava;										\
					kava.x = 7;										\
					kava.y = \"bonjour\";							\
					g0 = kova.y + kava.x + kava.y + kova.x;			\
				}													\
			}														\
			return g0;", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "hello7.00bonjour4.00" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Tables (Accessing properties from nested tables)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "Table x; Table y = { var x = \"NaNiSoRe\"; }; x.y = y; x.y.x = 4; return x.y.x;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 4 );
		TestUtils::FreeExitCode( exitCode );

		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0; {										\
				var l0 = 0;																	\
				Table x; Table y = { var x = \"NaNiSoRe\"; }; x.y = y;						\
				x.y.x = 5;																	\
				g0 = x.y.x;																	\
			} return g0;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 5 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Tables (Nested tables)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "Table a = {								\
				const x = 3;														\
				Table b = {															\
					const y = true;													\
					Table c = {														\
						const z = null;												\
					};																\
				};																	\
			};																		\
			return \"hello world\" + a.b.c.z + a.b.y + a.x;", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "hello world[[null]]True3.00");

		TestUtils::FreeExitCode( exitCode );

		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0; {									\
				var l0 = 0; var l1 = 0;													\
				Table a = {																\
					const x = 3;														\
					Table b = {															\
						const y = true;													\
						Table c = {														\
							const z = null;												\
						};																\
					};																	\
				};																		\
				g0 = \"hello world\" + a.b.c.z + a.b.y + a.x;							\
			}																			\
			return g0;", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "hello world[[null]]True3.00" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Tables (Simple methods)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0;				\
			{														\
				var l0 = 1;											\
				const l1 = 1;										\
				Table x = {											\
					const y = 9.0;									\
					Calc( num a, num b ) -> num {					\
						return a + b;								\
					};												\
					Calc2( num a ) -> {								\
						return a + this.y;							\
					};												\
				};													\
				const l2 = 1;										\
				g0 = [x.Calc 1, 3] * [x.Calc2 7.0];					\
			}														\
			return g0;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 64.0 );

		TestUtils::FreeExitCode( exitCode );

		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0;						\
		{																	\
			const num halfBase = 0.5;										\
			Table x ={														\
				const base = halfBase * 2.0;								\
				BaseNumber() -> num { return 1; }							\
				Fibo( a ) -> auto {											\
					if ( a <= this.base )									\
						return [this.BaseNumber];							\
					return [this.Fibo a - 1] + [this.Fibo a - 2];			\
				};															\
			};																\
			g0 = [x.Fibo 10];												\
		}																	\
		return g0;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 89.0 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Tables (Methods as first-class citizen)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0; var calc2;		\
			{														\
				var l0 = 1;											\
				const l1 = 1;										\
				Table x = {											\
					const y = 9.0;									\
					Calc( num a, num b ) -> num {					\
						return a + b;								\
					};												\
					Calc2( num a ) -> {								\
						return a + this.y;							\
					};												\
				};													\
				calc2 = x.Calc2;									\
				x.y = 5.0;											\
				const l2 = 1;										\
				const calculate = x.Calc;							\
				g0 = [calculate 1, l2] * [calc2 7.0];				\
			}														\
			return g0;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 24.0 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Tables (Anonymous tables)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0;			\
			{												\
				g0 = Table{									\
					const x = 6;							\
					const y = 7;							\
				};											\
			}												\
			return g0.x + g0.y; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 13.00 );

		TestUtils::FreeExitCode( exitCode );

		UTEST_ASSERT( TestUtils::RunVM( "var g0;			\
			{												\
				Array x ={									\
					Table { const a = 2; const b = 4; },	\
					2,										\
				};											\
				g0 = x(0).a + x(0).b + x(1);				\
			}												\
			return g0;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 8.00 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Arrays (Simple arrays)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0;				\
			{														\
				var l0 = 1;											\
				Array x = {	1, 2, 3 };								\
				const l2 = 1;										\
				g0 = x(0) + x(2);									\
			}														\
			return g0;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 4.0 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Arrays (Setters and getters)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0;				\
			{														\
				var l0 = 1;											\
				Array x = {	\"hello\", 2, 3};						\
				Array y = {	16, 32, \"world\" };					\
				Array z = {	128, \"home\", 512 };					\
				y(2) = z(1);										\
				const l2 = 1;										\
				g0 = x(0) + \" \" + y(2);							\
			}														\
			return g0;", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "hello home" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Arrays (Expression from setter)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0; var g1;		\
			{														\
				Array x = {	\"hello\", 2, 3};						\
				g0 = x(0) = x(0) + \" world\";						\
				g1 = x(0);											\
			}														\
			return g0 + \" \" + g1;", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "hello world hello world" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Arrays (Anonymous arrays)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0; {		\
				g0 = Array{ \"ano\", \"ny\", \"mous\" };	\
			}												\
			return g0(0) + g0(1) + g0(2); ", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "anonymous" );

		TestUtils::FreeExitCode( exitCode );

		UTEST_ASSERT( TestUtils::RunVM( "var g0;		\
			{											\
				Array named ={							\
					Array {								\
						Array { 1, 2, 3 },				\
					},									\
				};										\
				g0 = named(0)( 0 )(1);					\
			}											\
			return g0; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 2.00 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Arrays (STL Push)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0;		\
			{												\
				Array a;									\
				num l0 = 0;									\
				l0 +=[ a.push 1 ];							\
				l0 +=[ a.push \"hello\" ];					\
				l0 +=[ a.push \" \" ];						\
				l0 +=[ a.push \"world\" ];					\
				[a.push l0];								\
				g0 = a;										\
			}												\
			return g0;", &exitCode ) );

		UTEST_ASSERT( IS_ARRAY( exitCode ) );

		auto arr = AS_ARRAY( exitCode )->GetArray();

		UTEST_ASSERT( arr.size() == 5 );
		UTEST_ASSERT( IS_NUMBER( arr[ 0 ] ) && AS_NUMBER( arr[ 0 ] ) == 1.00 );
		UTEST_ASSERT( IS_STRING( arr[ 1 ] ) && AS_STRING( arr[ 1 ] )->GetString() == "hello" );
		UTEST_ASSERT( IS_STRING( arr[ 2 ] ) && AS_STRING( arr[ 2 ] )->GetString() == " " );
		UTEST_ASSERT( IS_STRING( arr[ 3 ] ) && AS_STRING( arr[ 3 ] )->GetString() == "world" );
		UTEST_ASSERT( IS_NUMBER( arr[ 4 ] ) && AS_NUMBER( arr[ 4 ] ) == 6.00 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Arrays (STL Concat)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0;		\
			{												\
				Array a ={ 1, 2, 3 };						\
				Array b ={ 4, \"5\", 6 };					\
				g0 =[ a.concat b ];							\
			}												\
			return g0; ", &exitCode ) );

		UTEST_ASSERT( IS_ARRAY( exitCode ) );

		auto arr = AS_ARRAY( exitCode )->GetArray();

		UTEST_ASSERT( arr.size() == 6 );
		UTEST_ASSERT( IS_NUMBER( arr[ 0 ] ) && AS_NUMBER( arr[ 0 ] ) == 1.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 1 ] ) && AS_NUMBER( arr[ 1 ] ) == 2.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 2 ] ) && AS_NUMBER( arr[ 2 ] ) == 3.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 3 ] ) && AS_NUMBER( arr[ 3 ] ) == 4.00 );
		UTEST_ASSERT( IS_STRING( arr[ 4 ] ) && AS_STRING( arr[ 4 ] )->GetString() == "5" );
		UTEST_ASSERT( IS_NUMBER( arr[ 5 ] ) && AS_NUMBER( arr[ 5 ] ) == 6.00 );

		TestUtils::FreeExitCode( exitCode );

		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0;		\
			{												\
				Array a ={ 1, 2, 3 };						\
				Array b ={ 4, \"5\", 6 };					\
				Array c ={ 7, 8, 9 };						\
				g0 = [a.concat b, c];						\
			}												\
			return g0; ", &exitCode ) );

		UTEST_ASSERT( IS_ARRAY( exitCode ) );

		arr = AS_ARRAY( exitCode )->GetArray();

		UTEST_ASSERT( arr.size() == 9 );
		UTEST_ASSERT( IS_NUMBER( arr[ 0 ] ) && AS_NUMBER( arr[ 0 ] ) == 1.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 1 ] ) && AS_NUMBER( arr[ 1 ] ) == 2.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 2 ] ) && AS_NUMBER( arr[ 2 ] ) == 3.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 3 ] ) && AS_NUMBER( arr[ 3 ] ) == 4.00 );
		UTEST_ASSERT( IS_STRING( arr[ 4 ] ) && AS_STRING( arr[ 4 ] )->GetString() == "5" );
		UTEST_ASSERT( IS_NUMBER( arr[ 5 ] ) && AS_NUMBER( arr[ 5 ] ) == 6.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 6 ] ) && AS_NUMBER( arr[ 6 ] ) == 7.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 7 ] ) && AS_NUMBER( arr[ 7 ] ) == 8.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 8 ] ) && AS_NUMBER( arr[ 8 ] ) == 9.00 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Arrays (STL Pop)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0;		\
			{												\
				Array a = { 1, \"2\", 3 };					\
				g0 = [a.pop] + [a.length];					\
			}												\
			return g0; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 5 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Arrays (STL Length)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0;			\
			{													\
				Array a ={ 1, 2, 3 };							\
				[a.push 4];										\
				Array b ={ 5, 6, 7 };							\
				g0 = Array { [ a.length ], [ b.length ] };		\
			}													\
			return g0;", &exitCode ) );

		UTEST_ASSERT( IS_ARRAY( exitCode ) );

		auto arr = AS_ARRAY( exitCode )->GetArray();
		UTEST_ASSERT( arr.size() == 2 );
		UTEST_ASSERT( IS_NUMBER( arr[ 0 ] ) && AS_NUMBER( arr[ 0 ] ) == 4.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 1 ] ) && AS_NUMBER( arr[ 1 ] ) == 3.00 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Arrays (STL Filter, basic filtering)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "Array a = { 5, 7, 1, 2, 9, 3 };			\
			var g;																	\
			{																		\
				Array b ={ 9, 7 };													\
				Array c ={ 6, 7, 8 };												\
				const filterTrice = ( item ) -> bool {								\
					return item == 7;												\
				}																	\
				const filterTwice = ( item ) -> {									\
					[c.filter filterTrice];											\
					for ( num i = 0; i < [ c.length ]; ++i ) {						\
						if ( c( i ) == item ) return true;							\
					}																\
					return false;													\
				}																	\
				const filterFn = ( item ) -> {										\
					const filtered =[ b.filter filterTwice ];						\
					for ( num i = 0; i < [ filtered.length ]; ++i ) {				\
						if ( filtered( i ) == item ) return true;					\
					}																\
					return false;													\
				};																	\
				[a.filter filterFn];												\
				g = [a.concat b, c];												\
			}																		\
			return g;", &exitCode ) );

		UTEST_ASSERT( IS_ARRAY( exitCode ) );

		auto arr = AS_ARRAY( exitCode )->GetArray();
		UTEST_ASSERT( arr.size() == 3 );
		UTEST_ASSERT( IS_NUMBER( arr[ 0 ] ) && AS_NUMBER( arr[ 0 ] ) == 7.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 1 ] ) && AS_NUMBER( arr[ 1 ] ) == 7.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 2 ] ) && AS_NUMBER( arr[ 2 ] ) == 7.00 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Arrays (STL Filter, use variables from outside filterFn)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "Array a = { 5, 7, 1, 2, 9, 3 };		\
			var g1 = 2;															\
			var g0;																\
			{																	\
				const filterFn = ( item ) -> {									\
					return item == g1;											\
				}																\
				g0 =[ a.filter filterFn ];										\
			}																	\
			return g0;", &exitCode ) );

		UTEST_ASSERT( IS_ARRAY( exitCode ) );

		auto arr = AS_ARRAY( exitCode )->GetArray();
		UTEST_ASSERT( arr.size() == 1 );
		UTEST_ASSERT( IS_NUMBER( arr[ 0 ] ) && AS_NUMBER( arr[ 0 ] ) == 2.00 );

		TestUtils::FreeExitCode( exitCode );

		UTEST_ASSERT( TestUtils::RunVM( "Array a = { 5, 7, 1, 2, 9, 3 };		\
			var g0;																\
			{																	\
				num l0 = 123;													\
				num l1 = 5;														\
				num l2 = 555;													\
				const filterFn = ( item ) -> {									\
					return item == l1;											\
				}																\
				g0 =[ a.filter filterFn ];										\
			}																	\
			return g0; ", &exitCode ) );

		UTEST_ASSERT( IS_ARRAY( exitCode ) );

		arr = AS_ARRAY( exitCode )->GetArray();
		UTEST_ASSERT( arr.size() == 1 );
		UTEST_ASSERT( IS_NUMBER( arr[ 0 ] ) && AS_NUMBER( arr[ 0 ] ) == 5.00 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Arrays (STL Filter, filterFn as a closure)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "Array a = { 5, 7, 1, 2, 9, 3 };		\
			var g0;																\
			{																	\
				const getFilterFn = ( ) -> {									\
					num numberToFilter = 3;										\
					const filterFn = ( item ) -> {								\
						return item == numberToFilter;							\
					}															\
					return filterFn;											\
				}																\
				g0 =[ a.filter[ getFilterFn ] ];								\
			}																	\
			return g0;", &exitCode ) );

		UTEST_ASSERT( IS_ARRAY( exitCode ) );

		auto arr = AS_ARRAY( exitCode )->GetArray();
		UTEST_ASSERT( arr.size() == 1 );
		UTEST_ASSERT( IS_NUMBER( arr[ 0 ] ) && AS_NUMBER( arr[ 0 ] ) == 3.00 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Arrays (STL Slice)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0;			\
			{												\
				Array a ={ 1, 2, 3, 4, 5, 6, 7 };			\
				g0 =[ [ a.slice 0 ].concat					\
					[ a.slice 2, 4 ],						\
					[ a.slice 5 ],							\
					[ a.slice 6, 123123 ],					\
					[ a.slice 6, 0 ],						\
					[ a.slice 1231 ]						\
				];											\
			}												\
			return g0;", &exitCode ) );

		UTEST_ASSERT( IS_ARRAY( exitCode ) );

		auto arr = AS_ARRAY( exitCode )->GetArray();
		UTEST_ASSERT( arr.size() == 12 );
		UTEST_ASSERT( IS_NUMBER( arr[ 0 ] ) && AS_NUMBER( arr[ 0 ] ) == 1.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 1 ] ) && AS_NUMBER( arr[ 1 ] ) == 2.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 2 ] ) && AS_NUMBER( arr[ 2 ] ) == 3.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 3 ] ) && AS_NUMBER( arr[ 3 ] ) == 4.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 4 ] ) && AS_NUMBER( arr[ 4 ] ) == 5.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 5 ] ) && AS_NUMBER( arr[ 5 ] ) == 6.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 6 ] ) && AS_NUMBER( arr[ 6 ] ) == 7.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 7 ] ) && AS_NUMBER( arr[ 7 ] ) == 3.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 8 ] ) && AS_NUMBER( arr[ 8 ] ) == 4.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 9 ] ) && AS_NUMBER( arr[ 9 ] ) == 6.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 10 ] ) && AS_NUMBER( arr[ 10 ] ) == 7.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 11 ] ) && AS_NUMBER( arr[ 11 ] ) == 7.00 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Arrays (STL Find)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0;		\
			{											\
				Array a ={ 1, 2, 3, 4, 5, 6, 7 };		\
				const filterFn = ( item ) -> {			\
					return item >= 4;					\
				};										\
				g0 =[ a.filter filterFn ];				\
			}											\
			return g0; ", &exitCode ) );

		UTEST_ASSERT( IS_ARRAY( exitCode ) );

		auto arr = AS_ARRAY( exitCode )->GetArray();
		UTEST_ASSERT( arr.size() == 4 );
		UTEST_ASSERT( IS_NUMBER( arr[ 0 ] ) && AS_NUMBER( arr[ 0 ] ) == 4.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 1 ] ) && AS_NUMBER( arr[ 1 ] ) == 5.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 2 ] ) && AS_NUMBER( arr[ 2 ] ) == 6.00 );
		UTEST_ASSERT( IS_NUMBER( arr[ 3 ] ) && AS_NUMBER( arr[ 3 ] ) == 7.00 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Mixing arrays and tables (1)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "Table a = {			\
			Table b = {											\
				Array x = { \"Hello\", \" \", \"world\" };		\
				Array y = { 0, 1, 2 };							\
			}													\
			Array z ={											\
				16,												\
				32,												\
				Table x = { const string e = \"!\"; }			\
			};													\
			Array q ={ Array v = { 1, 2 } };					\
		}														\
		return a.b.x( 0 )										\
			+ a.b.x( a.b.y( 1 ) )								\
			+ a.b.x( a.b.y( 2 ) )								\
			+ \" \"												\
			+ a.q( 0)( 1 )										\
			+ \"\"												\
			+ a.z(2).e; ", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "Hello world 2.00!" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Mixing arrays and tables (2)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "Table a ={					\
			Table b ={												\
				Array c = {											\
					0,												\
					Table d = {										\
						Array e = {									\
							1,										\
							2,										\
							Array f = {								\
								1,									\
								Array g = {							\
									Table h = {						\
										Array i = { \"big\" };		\
									}								\
								},									\
								3									\
							}										\
						};											\
					},												\
					2												\
				};													\
			};														\
		};															\
		return a.b.c(1).e(2)( 1 )( 0 ).i( 0 ); ", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "big" );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Nested arrays (Stack effect)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g0 = 0; const f = ( ) -> {		\
				Array out ={												\
					Array x,												\
				};															\
			for ( num i = 0; i < 3; ++i ) g0 = i;							\
		};																	\
		[f];																\
		return g0; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) = 2 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Nested arrays (Matrix multiplication)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "Array mat1 ={											\
				Array x0 ={ 2, 2, 2 },															\
				Array x1 ={ 2, 4, 2 },															\
				Array x2 ={ 2, 2, 2 },															\
			};																					\
			Array mat2 ={																		\
				Array x1 = { 6, 2, 6 },															\
				Array x2 = { 2, 6, 2 },															\
				Array x3 = { 6, 2, 6 },															\
			};																					\
			const matmul = ( a, b ) -> {														\
				Array out ={																	\
					Array x0 = { 0, 0, 0 },														\
					Array x1 = { 0, 0, 0 },														\
					Array x2 = { 0, 0, 0 },														\
				};																				\
				for ( num i = 0; i < 3; ++i ) {													\
					for ( num j = 0; j < 3; j++ ) {												\
						for ( num k = 0; k < 3; k++ ) {											\
							out( i )( j ) = out( i )( j ) + a( i )( k ) * b( k )( j );			\
						}																		\
					}																			\
				}																				\
				return out;																		\
			}																					\
			return [matmul mat1, mat2]; ", &exitCode ) );

		UTEST_ASSERT( IS_ARRAY( exitCode ) );

		auto arr = AS_ARRAY( exitCode )->GetArray();

#define ASSERT_INDICE( arr, x, y, number ) \
		UTEST_ASSERT( IS_ARRAY( arr[ x ] )	\
			&& IS_NUMBER( AS_ARRAY( arr[ x ] )->GetArray()[ y ] ) \
			&& ( AS_NUMBER( AS_ARRAY( arr[ x ] )->GetArray()[ y ] ) == number ) );

		ASSERT_INDICE( arr, 0, 0, 28.0 );
		ASSERT_INDICE( arr, 0, 1, 20.0 );
		ASSERT_INDICE( arr, 0, 2, 28.0 );

		ASSERT_INDICE( arr, 1, 0, 32.0 );
		ASSERT_INDICE( arr, 1, 1, 32.0 );
		ASSERT_INDICE( arr, 1, 2, 32.0 );

		ASSERT_INDICE( arr, 2, 0, 28.0 );
		ASSERT_INDICE( arr, 2, 1, 20.0 );
		ASSERT_INDICE( arr, 2, 2, 28.0 );

#undef ASSERT_INDICE
		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Nested arrays (Multi-level arrays)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "Array a1 = {			\
			Array a2 ={											\
				1,												\
				2,												\
				Array a3 = {									\
					1,											\
					Array a4 = {								\
						Array a5 = { 64.0 },					\
					},											\
					2											\
				}												\
			}													\
		}														\
		a1( 0 )( 2 )( 1 )( 0 )( 0 ) =							\
			a1( 0 )( 2 )( 1 )( 0 )( 0 )							\
			+ a1( 0 )( 2 )( 1 )( 0 )( 0 );						\
		return a1( 0 )( 2 )( 1 )( 0 )( 0 );", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) = 128.0 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Inline if -statements" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g1 = true ? 1 : 0;			\
			var g2 = (g1 != 1 ? 0 : 1);									\
			const x = () -> { return g2 == 1 ? \"yes\" : \"no\"; }		\
			return [x] == \"yes\" ? g1 + g2 : 0;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 2.0 );

		TestUtils::FreeExitCode( exitCode );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_END();
}
