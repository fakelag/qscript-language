#include "QLibPCH.h"
#include "Tests.h"

#include "../Library/Common/Chunk.h"
#include "../Library/Runtime/QVM.h"

#include "Utils.h"

using namespace Tests;

bool Tests::TestInterpreter()
{
	// Generate a body of more than 255 instructions
	auto largeBodyOfCode = TestUtils::GenerateSequence( 516, []( int iter ) {
		return "_tmp = _tmp+" + std::to_string( iter ) + std::string( ".00;" );
	}, "var _tmp = 0;" );

	auto largeExpression = TestUtils::GenerateSequence( 516, []( int iter ) {
		return "0.00" + std::string( iter == 515 ? "" : "+" );
	} );

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

		UTEST_ASSERT( TestUtils::RunVM( "var x = 0;								\
				if (x) { " + largeBodyOfCode + " }								\
				else if (x == 1) { " + largeBodyOfCode + " }					\
				else { " + largeBodyOfCode + " x = 42; }						\
			return x;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 42 );

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

		UTEST_ASSERT( TestUtils::RunVM( "var g1 = 0;			\
				while ((g1 = g1 + 1) < 40) {}						\
			return g1;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 40.0 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "do-while clause" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var f = 100; do { f = f - 1; } while (f > 50); return f;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 50 );

		UTEST_ASSERT( TestUtils::RunVM( "var g = 0;		\
			var g1 = 0;									\
			while ( g < 10 ) {							\
				var l0 = 0;								\
				var l1 = 25;							\
				do {									\
					l0 = l0 + l1;						\
				} while ( l0 < 100 );					\
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
				} while ( ( iter = iter + 1 ) < 50 );		\
			}												\
			return g1; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 20365011074.0 );

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

		UTEST_ASSERT( TestUtils::RunVM( "var g = 1;																			\
			for ( var i = " + largeExpression + "; i < 10 +" + largeExpression + "; i = i + 1 + " + largeExpression	+ ") {	\
				" + largeBodyOfCode + " g = i + 1;																			\
			} \
			return g;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 10 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE_1( "functions 1 (Simple calls, large functions, args, return values)", &largeBodyOfCode )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var x = 5;				\
			function abc = () {									\
				var x = 5;										\
				var y = x + 50;									\
				{ var z = y + y; }								\
				var q = x;										\
			}													\
			abc();												\
			return 1;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 1 );

		UTEST_ASSERT( TestUtils::RunVM( "var x = 5;				\
			function abc = (x) {								\
				var y = x + 50;									\
				{ " + largeBodyOfCode + " }						\
				var q = y + x;									\
				return q;										\
			}													\
			" + largeBodyOfCode + "								\
			x = abc(5);											\
			return x;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 60 );

		UTEST_ASSERT( TestUtils::RunVM( "var x = 5;				\
			function abc = (a, b, c) {							\
				return a + b + c;								\
			}													\
			return abc(7, 9, 2);", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 18 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "functions 2 (Calling functions bound to variables, recursive functions)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var x = 0;				\
			function a = (x) { var y = x + 5; return y + 5; }	\
			function b = (x) { return a(x) + 5; }				\
			function c = (x) { return b(x) - 5; }				\
			x = c(20);											\
			return x;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 30 );

		UTEST_ASSERT( TestUtils::RunVM( "						\
			function fibonacci = (num) {						\
				if (num <= 1) return 1;							\
				return fibonacci(num - 1) + fibonacci(num - 2);	\
			}													\
			return fibonacci(10);", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 89 );

		UTEST_ASSERT( TestUtils::RunVM( "var x = 0;							\
			function xxx = (anotherFunc) { return anotherFunc(60, 60); }	\
			function sum = (a, b) { return a + b; }							\
			return xxx(sum);", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 120 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Closures 1 (Simple closures)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "function f = () {		\
			var l0 = \"initial\";								\
			function a = ( ) {									\
				l0 = \"second\";								\
			}													\
			a();												\
			return l0;											\
		}														\
		return f();", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "second" );

		UTEST_ASSERT( TestUtils::RunVM( "function f = () {		\
			var l0 = \"returned\";								\
			function a = ( ) {									\
				return l0;										\
			}													\
			return a();											\
		}														\
		return f();", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "returned" );

		UTEST_ASSERT( TestUtils::RunVM( "function g = () {		\
			var a = \"a\";										\
			var b = \"b\";										\
			function f = ( ) {									\
				return a + b;									\
			}													\
			return f();											\
		}														\
		return g();", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "ab" );

		UTEST_ASSERT( TestUtils::RunVM( "var g = null;		\
		{													\
			var l0 = 333;									\
			var l1 = 919191;								\
			var l2 = 222;									\
			function l3 = ( ) {								\
				return l1;									\
			}												\
			g = l3;											\
		}													\
		return g(); ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 919191 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Closures 2 (Nested closures, reuse stack slot, shadowing)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "var g = null;		\
			function a = ( ) {								\
			var mmmm;										\
			var l0 = 1;										\
			var bbbb;										\
			function b = ( ) {								\
				var oooo;									\
				var l1 = 2;									\
				var ffff;									\
				function c = ( ) {							\
					var xxx;								\
					var l2 = 3;								\
					var zzz;								\
					var ggg;								\
					function d = ( ) {						\
						g = l0 + l1 + l2;					\
						g = g + l0;							\
					}										\
					d();									\
				}											\
				c();										\
			}												\
			b();											\
		}													\
		a();												\
		return g; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 7 );

		UTEST_ASSERT( TestUtils::RunVM( "var g = null;			\
			var g1 = null;										\
			{													\
				var l0 = null;									\
				{												\
					var a = 1;									\
					function retA = ( ) { return a; }			\
					g = retA;									\
				}												\
				{												\
					var b = 2;									\
					g1 = g();									\
				}												\
			} return g1;", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 1 );

		UTEST_ASSERT( TestUtils::RunVM( "var g = \"\";			\
			function process = ( ) {							\
			var l0 = \"closure\";								\
			function l1 = ( ) {									\
				g = g + l0;										\
				var l0 = \"shadow\";							\
				g = g + l0;										\
			}													\
			l1();												\
			g = g + l0;											\
		}														\
		process();												\
		return g; ", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "closureshadowclosure" );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Closures 3 (Unused closure, close over function param, refer to same variable on all closures)" )
	{
		QScript::Value exitCode;
		UTEST_ASSERT( TestUtils::RunVM( "function g = () {		\
				var a = \"object\";								\
				if ( false ) { function x = () { return a; } }	\
			}													\
			g();												\
			return 6; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 6 );

		UTEST_ASSERT( TestUtils::RunVM( "function g = () {		\
				var a = \"object\";								\
				if ( false ) { function x = () { return a; } }	\
			}													\
			g();												\
			return 6; ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 6 );

		UTEST_ASSERT( TestUtils::RunVM( "var g;							\
			var g2;														\
			{															\
				var a = \"a\";											\
				{														\
					var b = \"b\";										\
					function x = ( ) { return a; }						\
					g = x;												\
					if ( false ) { function z = ( ) { return b; } }		\
				}														\
				g2 = g();												\
			}															\
			return g2; ", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "a" );

		UTEST_ASSERT( TestUtils::RunVM( "var g = null;					\
			function f = ( arg0 ) {										\
				function x = ( ) {										\
					return arg0;										\
				}														\
				g = x;													\
			}															\
			f( \"hello\" );												\
			return g(); ", &exitCode ) );

		UTEST_ASSERT( IS_STRING( exitCode ) );
		UTEST_ASSERT( AS_STRING( exitCode )->GetString() == "hello" );

		UTEST_ASSERT( TestUtils::RunVM( "var gAdd;					\
			var gGet;												\
			function g = ( ) {										\
				var v = 0;											\
				function add = ( ) { v = v + 1; }					\
				function get = ( ) { return v; }					\
				gAdd = add;											\
				gGet = get;											\
			}														\
			g();													\
			gAdd();													\
			gAdd();													\
			return gGet(); ", &exitCode ) );

		UTEST_ASSERT( IS_NUMBER( exitCode ) );
		UTEST_ASSERT( AS_NUMBER( exitCode ) == 2 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_END();
}
