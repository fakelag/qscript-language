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
	UTEST_BEGIN( "Compiler Tests" );

	UTEST_CASE( "Generate simple bytecode" )
	{
		auto fn = QScript::Compile( "return 2 + 2;" );

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

		fn = QScript::Compile( "var k; k = 10; k = k + 1; print k;" );
		QScript::FreeFunction( fn );

		UTEST_THROW_EXCEPTION( QScript::Compile( "k = 10;" ),
			const std::vector< CompilerException >& e,
			e.size() == 1 && e[ 0 ].id() == "cp_unknown_identifier" );

		UTEST_THROW_EXCEPTION( QScript::Compile( "print k;" ),
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

	UTEST_CASE( "Constant stacking" )
	{
		QScript::Config_t config;
		config.m_OptFlags |= QScript::Config_t::OF_CONSTANT_STACKING;

		auto fn = QScript::Compile( TestUtils::GenerateSequence( 512, []( int iter ) {
			return "+" + std::to_string( iter % 2 == 0 ? iter : 8000 ) + std::string( ".00" );
		}, "var f = 0.00 ", ";" ), config );

		UTEST_ASSERT( fn->GetChunk()->m_Constants.size() == 258 );

		QScript::FreeFunction( fn );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_END();
}
