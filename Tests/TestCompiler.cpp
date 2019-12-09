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
		auto chunk = QScript::Compile( "2 + 2;" );

		/*
			LOAD 2.0
			LOAD 2.0
			ADD
			RETN
		*/

		UTEST_ASSERT( chunk->m_Code.size() == 6 );
		UTEST_ASSERT( chunk->m_Code[ 0 ] == QScript::OpCode::OP_LOAD );
		UTEST_ASSERT( chunk->m_Code[ 2 ] == QScript::OpCode::OP_LOAD );
		UTEST_ASSERT( chunk->m_Code[ 4 ] == QScript::OpCode::OP_ADD );
		UTEST_ASSERT( chunk->m_Code[ 5 ] == QScript::OpCode::OP_RETN );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_END();
}
