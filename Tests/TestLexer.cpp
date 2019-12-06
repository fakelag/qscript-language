#include "QLibPCH.h"
#include "Tests.h"

// Include code
#include "../Library/Common/Chunk.h"
#include "../Library/Compiler/Compiler.h"

using namespace Tests;
using namespace Compiler;

bool Tests::TestLexer()
{
	UTEST_BEGIN( "Lexer Tests" );

	UTEST_CASE( "Generate tokens from a sentence" )
	{
		auto tokens = Lexer( "2 + 4.2 * 4 - 5 / 10 * 20;" );

		UTEST_ASSERT( tokens.size() == 12 );
		UTEST_ASSERT( tokens[ 0 ].m_Token == TOK_INT );
		UTEST_ASSERT( tokens[ 1 ].m_Token == TOK_PLUS );
		UTEST_ASSERT( tokens[ 2 ].m_Token == TOK_DBL );
		UTEST_ASSERT( tokens[ 3 ].m_Token == TOK_STAR );
		UTEST_ASSERT( tokens[ 4 ].m_Token == TOK_INT );
		UTEST_ASSERT( tokens[ 5 ].m_Token == TOK_MINUS );
		UTEST_ASSERT( tokens[ 6 ].m_Token == TOK_INT );
		UTEST_ASSERT( tokens[ 7 ].m_Token == TOK_SLASH );
		UTEST_ASSERT( tokens[ 8 ].m_Token == TOK_INT );
		UTEST_ASSERT( tokens[ 9 ].m_Token == TOK_STAR );
		UTEST_ASSERT( tokens[ 10 ].m_Token == TOK_INT );
		UTEST_ASSERT( tokens[ 11 ].m_Token == TOK_SCOLON );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Ignore tabs and newlines in the code" )
	{
		auto symbols = Lexer( "\t5 \n+ \t3* 1\n\n- 2; \n				\
			1 + 2;														\
			2 + 3;														\
			3 + 4;														\
		" );

		UTEST_ASSERT( symbols.size() == 20 );
		UTEST_ASSERT( symbols[ 0 ].m_String == "5" );
		UTEST_ASSERT( symbols[ 1 ].m_Token == TOK_PLUS );
		UTEST_ASSERT( symbols[ 5 ].m_Token == TOK_MINUS );
		UTEST_ASSERT( symbols[ 12 ].m_String == "2" );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Ignore comments" )
	{
		auto symbols = Lexer( "			\
			1+1	// this is a comment\n	\
			1+1	/* another one */ 2+2	\
			/* aaaaa					\
				bbbb					\
			*/							\
			1+1							\
		" );

		UTEST_ASSERT( symbols.size() == 12 );
		UTEST_ASSERT( symbols[ 4 ].m_String == "+" );
		UTEST_ASSERT( symbols[ 6 ].m_String == "2" );
		UTEST_ASSERT( symbols[ 7 ].m_String == "+" );
		UTEST_ASSERT( symbols[ 9 ].m_String == "1" );
		UTEST_ASSERT( symbols[ 10 ].m_String == "+" );

		symbols = Lexer( "// comment only" );
		UTEST_ASSERT( symbols.size() == 0 );

		symbols = Lexer( "/* comment only */" );
		UTEST_ASSERT( symbols.size() == 0 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Multicharacter symbols" )
	{
		auto symbols = Lexer( "5 + 2 == 3 * 2;" );

		UTEST_ASSERT( symbols.size() == 8 );
		UTEST_ASSERT( symbols[ 3 ].m_Token == TOK_2EQUALS );
		UTEST_ASSERT( symbols[ 3 ].m_String == "==" );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Debug info generation" )
	{
		auto symbols = Lexer( "5 \n+ 2 != 3\n * 2+2\n*1;" );

		UTEST_ASSERT( symbols.size() == 12 );
		UTEST_ASSERT( symbols[ 0 ].m_ColNr == 0 );
		UTEST_ASSERT( symbols[ 0 ].m_LineNr == 0 );
		UTEST_ASSERT( symbols[ 1 ].m_ColNr == 0 );
		UTEST_ASSERT( symbols[ 1 ].m_LineNr == 1 );
		UTEST_ASSERT( symbols[ 2 ].m_ColNr == 2 );
		UTEST_ASSERT( symbols[ 2 ].m_LineNr == 1 );
		UTEST_ASSERT( symbols[ 3 ].m_ColNr == 4 );
		UTEST_ASSERT( symbols[ 3 ].m_LineNr == 1 );
		UTEST_ASSERT( symbols[ 6 ].m_ColNr == 3 );
		UTEST_ASSERT( symbols[ 6 ].m_LineNr == 2 );
		UTEST_ASSERT( symbols[ 7 ].m_ColNr == 4 ); // +
		UTEST_ASSERT( symbols[ 7 ].m_LineNr == 2 );
		UTEST_ASSERT( symbols[ 8 ].m_ColNr == 5 ); // 2
		UTEST_ASSERT( symbols[ 8 ].m_LineNr == 2 );
		UTEST_ASSERT( symbols[ 9 ].m_ColNr == 0 ); // *
		UTEST_ASSERT( symbols[ 9 ].m_LineNr == 3 );
		UTEST_ASSERT( symbols[ 10 ].m_ColNr == 1 ); // 1
		UTEST_ASSERT( symbols[ 10 ].m_LineNr == 3 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "String constants" )
	{
		auto tokens = Lexer( "a + \"this is a /*string*/ constant // comments dont apply\";" );

		UTEST_ASSERT( tokens.size() == 4 );
		UTEST_ASSERT( tokens[ 0 ].m_Token == TOK_NAME );
		UTEST_ASSERT( tokens[ 1 ].m_Token == TOK_PLUS );
		UTEST_ASSERT( tokens[ 2 ].m_Token == TOK_STR );
		UTEST_ASSERT( tokens[ 2 ].m_String == "this is a /*string*/ constant // comments dont apply" );
		UTEST_ASSERT( tokens[ 3 ].m_Token == TOK_SCOLON );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_END();
}
