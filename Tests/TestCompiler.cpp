#include "QLibPCH.h"
#include "Tests.h"
#include "Instructions.h"

#include "../Library/Common/Chunk.h"
#include "../Library/Compiler/Compiler.h"

using namespace Tests;
using namespace Compiler;

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
		UTEST_ASSERT( chunk->m_Code[ 0 ] == QScript::OpCode::OP_LOAD );
		UTEST_ASSERT( chunk->m_Code[ 2 ] == QScript::OpCode::OP_LOAD );
		UTEST_ASSERT( chunk->m_Code[ 4 ] == QScript::OpCode::OP_ADD );
		UTEST_ASSERT( chunk->m_Code[ 5 ] == QScript::OpCode::OP_RETN );

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

	UTEST_END();
}
