#include "QLibPCH.h"
#include "Tests.h"
#include "Instructions.h"

#include "../Library/Common/Chunk.h"
#include "../Library/Compiler/Compiler.h"

#include "Utils.h"

using namespace Tests;
using namespace Compiler;

#define ASSERT_OPCODE( address, opcode ) \
int opcodeOffset = Compiler::InstructionSize( fn->GetChunk()->m_Code[ address ] ); \
UTEST_ASSERT( fn->GetChunk()->m_Code[ address ] == QScript::OpCode::opcode )

#define ASSERT_OPCODE_NEXT( opcode ) \
UTEST_ASSERT( fn->GetChunk()->m_Code[ opcodeOffset ] == QScript::OpCode::opcode ) \
opcodeOffset += Compiler::InstructionSize( fn->GetChunk()->m_Code[ opcodeOffset ] ); \

bool Tests::TestCompiler()
{
	static const int s_LargeConstCount = 300;

	UTEST_BEGIN( "Compiler Tests" );

	UTEST_CASE( "Generate simple bytecode" )
	{
		auto fn = QScript::Compile( "return 2.5 + 2.5;" );

		/*
			LOAD 2.0
			LOAD 2.0
			ADD
			RETN
		*/

		UTEST_ASSERT( fn->GetChunk()->m_Code.size() >= 6 );

		ASSERT_OPCODE( 0000, OP_LOAD_CONSTANT_SHORT );
		ASSERT_OPCODE_NEXT( OP_LOAD_CONSTANT_SHORT );
		ASSERT_OPCODE_NEXT( OP_ADD );
		ASSERT_OPCODE_NEXT( OP_RETURN );

		QScript::FreeFunction( fn );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Handle multiple exceptions" )
	{
		UTEST_THROW_EXCEPTION( QScript::Compile( "2 / / ; 2 / /; 2 / /;" ),
			const std::vector< CompilerException >& e,
			e.size() == 3 && e[ 0 ].id() == std::string( "ir_expect_lvalue_or_statement" )
				&& e[ 1 ].id() == std::string( "ir_expect_lvalue_or_statement" )
				&& e[ 2 ].id() == std::string( "ir_expect_lvalue_or_statement" ) );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Known, unknown and redefined globals" )
	{
		auto fn = QScript::Compile( "var known123 = 10;" );
		QScript::FreeFunction( fn );

		fn = QScript::Compile( "var k; k = 10; k = k + 1; print(k);" );
		QScript::FreeFunction( fn );

		UTEST_THROW_EXCEPTION( QScript::Compile( "k = 10;" ),
			const std::vector< CompilerException >& e,
			e.size() == 1 && e[ 0 ].id() == "cp_unknown_identifier" );

		UTEST_THROW_EXCEPTION( QScript::Compile( "print(k);" ),
			const std::vector< CompilerException >& e,
			e.size() == 1 && e[ 0 ].id() == "cp_unknown_identifier" );

		UTEST_THROW_EXCEPTION( QScript::Compile( "var k; var k;" ),
			const std::vector< CompilerException >& e,
			e.size() == 1 && e[ 0 ].id() == "cp_identifier_already_exists" );

		UTEST_THROW_EXCEPTION( QScript::Compile( "var k = 0; var k;" ),
			const std::vector< CompilerException >& e,
			e.size() == 1 && e[ 0 ].id() == "cp_identifier_already_exists" );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Constant variables" )
	{
		auto fn = QScript::Compile( "const x; const y = 10 * 10 + x;" );
		QScript::FreeFunction( fn );

		fn = QScript::Compile( "{ const x = 10; const z; print(z); }" );
		QScript::FreeFunction( fn );

		fn = QScript::Compile( "const z = () -> { const localVal = 10; 	\
		const localFunc = () -> { print(localVal + 1); } 				\
		}" );
		QScript::FreeFunction( fn );

		UTEST_THROW_EXCEPTION( QScript::Compile( "const x = 1; x = 2;" ),
			const std::vector< CompilerException >& e,
			e.size() == 1 && e[ 0 ].id() == "cp_assign_to_const" );

		UTEST_THROW_EXCEPTION( QScript::Compile( "const z = () -> { const localVal = 10; 	\
			const localFunc = () -> { localVal = 1; } 										\
			}" ),
			const std::vector< CompilerException >& e,
			e.size() == 1 && e[ 0 ].id() == "cp_assign_to_const" );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Valid/Invalid assignment targets (--, ++, =, +=, -=, *=, /=, %=)" )
	{
		#define ASSIGN_VALID( pre, post, operand ) \
			QScript::FreeFunction( QScript::Compile( "var x = 0;" pre "x" post operand ";" ) );

		#define ASSIGN_INVALID( pre, post, operand ) \
			UTEST_THROW_EXCEPTION( QScript::Compile( pre "1" post operand ";" ), \
				const std::vector< CompilerException >& e, \
				e.size() == 1 && e[ 0 ].id() == "cp_invalid_assign_target" );

		ASSIGN_VALID( "++", "", "" );
		ASSIGN_VALID( "", "++", "" );
		ASSIGN_VALID( "--", "", "" );
		ASSIGN_VALID( "", "--", "" );

		ASSIGN_VALID( "", "=", "42" );
		ASSIGN_VALID( "", "+=", "42" );
		ASSIGN_VALID( "", "-=", "42" );
		ASSIGN_VALID( "", "*=", "42" );
		ASSIGN_VALID( "", "/=", "42" );
		ASSIGN_VALID( "", "%=", "42" );

		ASSIGN_INVALID( "++", "", "" );
		ASSIGN_INVALID( "", "++", "" );
		ASSIGN_INVALID( "--", "", "" );
		ASSIGN_INVALID( "", "--", "" );

		ASSIGN_INVALID( "", "+=", "42" );
		ASSIGN_INVALID( "", "-=", "42" );
		ASSIGN_INVALID( "", "*=", "42" );
		ASSIGN_INVALID( "", "/=", "42" );
		ASSIGN_INVALID( "", "%=", "42" );

		#undef ASSIGN_VALID
		#undef ASSIGN_INVALID
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Constant stacking" )
	{
		auto fn = QScript::Compile( TestUtils::GenerateSequence( s_LargeConstCount, []( int iter ) {
			return "+" + std::to_string( iter % 2 == 0 ? iter : 8000 ) + std::string( ".00" );
		}, "var f = 0.00 ", ";" ) );

		UTEST_ASSERT( fn->GetChunk()->m_Constants.size() <= 258 );

		QScript::FreeFunction( fn );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Typed variables" )
	{
		QScript::FreeFunction( QScript::Compile( "num a = 4 * 4; 		\
			string b = \"abcdefg\";										\
			bool c = true;" ) );

		QScript::FreeFunction( QScript::Compile( "const num a = 4 * 4; 		\
			const string b = \"abcdefg\";									\
			const bool c = true;" ) );

		UTEST_THROW_EXCEPTION( QScript::Compile( "num x = true;" ),
			const std::vector< CompilerException >& e,
			e.size() == 1 && e[ 0 ].id() == "cp_invalid_expression_type" );

		UTEST_THROW_EXCEPTION( QScript::Compile( "const num x = true;" ),
			const std::vector< CompilerException >& e,
			e.size() == 1 && e[ 0 ].id() == "cp_invalid_expression_type" );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Expression type checks" )
	{
		UTEST_ASSERT( QScript::Typer( "auto x = 4 - 4 + 2 * 2;" )[ 0 ].first == TYPE_NUMBER );

		UTEST_ASSERT( QScript::Typer( "auto x = (4 - 4) + 2 + \"2\";" )[ 0 ].first == TYPE_STRING );

		UTEST_THROW_EXCEPTION( QScript::Compile( "var x = 2 - \"str\";" ),
			const std::vector< CompilerException >& e,
			e.size() == 1 && e[ 0 ].id() == "cp_invalid_expression_type" );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Function return types" )
	{
		UTEST_ASSERT( QScript::Typer( "const auto x = (num a, string b) -> { return a + b; }" )[ 0 ].second == TYPE_UNKNOWN );

		UTEST_ASSERT( ( QScript::Typer( "const auto x = (num a, string b) -> auto { return a + b; }" )[ 0 ].second & TYPE_STRING ) == TYPE_STRING );

		UTEST_ASSERT( ( QScript::Typer( "const auto x = (num a, num b) -> auto { return a + b; }" )[ 0 ].second & TYPE_NUMBER ) == TYPE_NUMBER );

		UTEST_THROW_EXCEPTION( QScript::Compile( "const x = () -> num { return \"abcdefg\"; }" ),
			const std::vector< CompilerException >& e,
			e.size() == 1 && e[ 0 ].id() == "cp_invalid_expression_type" );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Function args types" )
	{
		QScript::FreeFunction( QScript::Compile( "const x = (num a, string b) -> {	\
				return a + b;														\
			}" ) );

		UTEST_THROW_EXCEPTION( QScript::Compile( "const x = (num a, string b) -> {	\
				return a - b;														\
			}" ),
			const std::vector< CompilerException >& e,
			e.size() == 1 && e[ 0 ].id() == "cp_invalid_expression_type" );

		UTEST_THROW_EXCEPTION( QScript::Compile( "const x = (xxx) -> { xxx = 4; }" ),
			const std::vector< CompilerException >& e,
			e.size() == 1 && e[ 0 ].id() == "cp_assign_to_const" );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_END();
}
