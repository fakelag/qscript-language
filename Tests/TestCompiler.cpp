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
		auto chunk = QScript::Compile( TestUtils::GenerateSequence( 512, []( int iter ) {
			return std::to_string( iter ) + std::string( ".00;" );
		} ) );

		UTEST_ASSERT( chunk->m_Constants.size() == 512 );

		/*
			Sequences of
			0000 OP_LOAD_SHORT
			0001 index
			0002 OP_POP
			...
			0000 OP_LOAD_LONG
			0001 index
			0002 index
			0003 index
			0004 index
			0005 OP_POP
		*/

		ASSERT_OPCODE( 0000, OP_LD_SHORT );
		ASSERT_OPCODE_NEXT( OP_POP );

		int firstLong = 255 * 3;
		UTEST_ASSERT( chunk->m_Code[ firstLong ] == QScript::OpCode::OP_LD_LONG );
		UTEST_ASSERT( chunk->m_Code[ firstLong + 5 ] == QScript::OpCode::OP_POP );

		QScript::FreeChunk( chunk );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Assign constants" )
	{
		auto chunk = QScript::Compile( "var a; var b = 4; a = 32.2;" );

		UTEST_ASSERT( chunk->m_Constants.size() == 5 );

		ASSERT_OPCODE( 0000, OP_PNULL );
		ASSERT_OPCODE_NEXT( OP_SG_SHORT );
		ASSERT_OPCODE_NEXT( OP_POP );
		ASSERT_OPCODE_NEXT( OP_LD_SHORT );
		ASSERT_OPCODE_NEXT( OP_SG_SHORT );
		ASSERT_OPCODE_NEXT( OP_POP );

		ASSERT_OPCODE_NEXT( OP_LD_SHORT );
		ASSERT_OPCODE_NEXT( OP_SG_SHORT );
		ASSERT_OPCODE_NEXT( OP_POP );

		ASSERT_OPCODE_NEXT( OP_RETN );

		QScript::FreeChunk( chunk );
		UTEST_CASE_CLOSED();
	}( );

	UTEST_END();
}
