#include "QLibPCH.h"
#include "Tests.h"
#include "Instructions.h"

#include "../Library/Common/Chunk.h"
#include "../Library/Compiler/Compiler.h"

#include "Utils.h"

using namespace Tests;
using namespace Compiler;

#define ASSERT_OPCODE( address, opcode ) \
int opcodeOffset = Compiler::InstructionSize( chunk->m_Code[ address ] ); \
UTEST_ASSERT( chunk->m_Code[ address ] == QScript::OpCode::opcode )

#define ASSERT_OPCODE_NEXT( opcode ) \
UTEST_ASSERT( chunk->m_Code[ opcodeOffset ] == QScript::OpCode::opcode ) \
opcodeOffset += Compiler::InstructionSize( chunk->m_Code[ opcodeOffset ] ); \

bool Tests::TestCompiler()
{
	UTEST_BEGIN( "Compiler Tests" );

	UTEST_CASE( "Generate simple bytecode" )
	{
		auto chunk = QScript::Compile( "return 2 + 2;" );

		/*
			LOAD 2.0
			LOAD 2.0
			ADD
			RETN
		*/

		UTEST_ASSERT( chunk->m_Code.size() >= 6 );

		ASSERT_OPCODE( 0000, OP_LD_SHORT );
		ASSERT_OPCODE_NEXT( OP_LD_SHORT );
		ASSERT_OPCODE_NEXT( OP_ADD );
		ASSERT_OPCODE_NEXT( OP_RETN );

		QScript::FreeChunk( chunk );
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

	UTEST_CASE( "Compile > 255 constants" )
	{
		std::string sourceCode = TestUtils::GenerateSequence( 512, []( int iter ) {
			return std::to_string( iter ) + std::string( ".00;" );
		} );

		auto chunk = QScript::Compile( sourceCode );

		UTEST_ASSERT( chunk->m_Constants.size() == 512 );

		ASSERT_OPCODE( 0000, OP_LD_SHORT );
		ASSERT_OPCODE_NEXT( OP_POP );

		int firstLong = 256 * 3;
		UTEST_ASSERT( chunk->m_Code[ firstLong ] == QScript::OpCode::OP_LD_LONG );
		UTEST_ASSERT( chunk->m_Code[ firstLong + 5 ] == QScript::OpCode::OP_POP );

		opcodeOffset = 0;
		QScript::FreeChunk( chunk );

		chunk = QScript::Compile( sourceCode, QScript::OF_CONSTANT_STACKING );

		UTEST_ASSERT( chunk->m_Constants.size() == 512 );

		ASSERT_OPCODE_NEXT( OP_LD_SHORT );
		ASSERT_OPCODE_NEXT( OP_POP );

		firstLong = 256 * 3;
		UTEST_ASSERT( chunk->m_Code[ firstLong ] == QScript::OpCode::OP_LD_LONG );
		UTEST_ASSERT( chunk->m_Code[ firstLong + 5 ] == QScript::OpCode::OP_POP );

		QScript::FreeChunk( chunk );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Constant stacking" )
	{
		auto chunk = QScript::Compile( TestUtils::GenerateSequence( 512, []( int iter ) {
			return std::to_string( iter % 2 == 0 ? iter : 8000 ) + std::string( ".00;" );
		} ), QScript::OF_CONSTANT_STACKING );

		UTEST_ASSERT( chunk->m_Constants.size() == 257 );

		QScript::FreeChunk( chunk );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Assign constants to global variables" )
	{
		auto chunk = QScript::Compile( "var a; var b = 4; a = 32.2;" );

		UTEST_ASSERT( chunk->m_Constants.size() == 5 );

		ASSERT_OPCODE( 0000, OP_PNULL );
		ASSERT_OPCODE_NEXT( OP_SG_SHORT );
		ASSERT_OPCODE_NEXT( OP_LD_SHORT );
		ASSERT_OPCODE_NEXT( OP_SG_SHORT );

		ASSERT_OPCODE_NEXT( OP_LD_SHORT );
		ASSERT_OPCODE_NEXT( OP_SG_SHORT );

		ASSERT_OPCODE_NEXT( OP_RETN );

		QScript::FreeChunk( chunk );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_END();
}
