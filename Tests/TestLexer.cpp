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

	UTEST_CASE( "Generate tokens from a sentence (1)" )
	{
		auto tokens = Lexer( "2 + 4.2 * 4 - 5 / 10 * 20;" );

		UTEST_ASSERT( tokens.size() == 12 );
		UTEST_ASSERT( tokens[ 0 ].m_Id == TOK_INT );
		UTEST_ASSERT( tokens[ 1 ].m_Id == TOK_PLUS );
		UTEST_ASSERT( tokens[ 2 ].m_Id == TOK_DBL );
		UTEST_ASSERT( tokens[ 3 ].m_Id == TOK_STAR );
		UTEST_ASSERT( tokens[ 4 ].m_Id == TOK_INT );
		UTEST_ASSERT( tokens[ 5 ].m_Id == TOK_MINUS );
		UTEST_ASSERT( tokens[ 6 ].m_Id == TOK_INT );
		UTEST_ASSERT( tokens[ 7 ].m_Id == TOK_SLASH );
		UTEST_ASSERT( tokens[ 8 ].m_Id == TOK_INT );
		UTEST_ASSERT( tokens[ 9 ].m_Id == TOK_STAR );
		UTEST_ASSERT( tokens[ 10 ].m_Id == TOK_INT );
		UTEST_ASSERT( tokens[ 11 ].m_Id == TOK_SCOLON );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Generate tokens from a sentence (2)" )
	{
		auto tokens = Lexer( "const numbaHalf = 4+ halfnumba-3.213+3+2.232+2.232 num; }while(true)" );

		UTEST_ASSERT( tokens.size() == 21 );
		UTEST_ASSERT( tokens[ 0 ].m_Id == TOK_CONST );
		UTEST_ASSERT( tokens[ 1 ].m_Id == TOK_NAME );
		UTEST_ASSERT( tokens[ 2 ].m_Id == TOK_EQUALS );
		UTEST_ASSERT( tokens[ 3 ].m_Id == TOK_INT );
		UTEST_ASSERT( tokens[ 4 ].m_Id == TOK_PLUS );
		UTEST_ASSERT( tokens[ 5 ].m_Id == TOK_NAME );
		UTEST_ASSERT( tokens[ 6 ].m_Id == TOK_MINUS );
		UTEST_ASSERT( tokens[ 7 ].m_Id == TOK_DBL );
		UTEST_ASSERT( tokens[ 8 ].m_Id == TOK_PLUS );
		UTEST_ASSERT( tokens[ 9 ].m_Id == TOK_INT );
		UTEST_ASSERT( tokens[ 10 ].m_Id == TOK_PLUS );
		UTEST_ASSERT( tokens[ 11 ].m_Id == TOK_DBL );
		UTEST_ASSERT( tokens[ 12 ].m_Id == TOK_PLUS );
		UTEST_ASSERT( tokens[ 13 ].m_Id == TOK_DBL );
		UTEST_ASSERT( tokens[ 14 ].m_Id == TOK_NUMBER );
		UTEST_ASSERT( tokens[ 15 ].m_Id == TOK_SCOLON );
		UTEST_ASSERT( tokens[ 16 ].m_Id == TOK_BRACE_RIGHT );
		UTEST_ASSERT( tokens[ 17 ].m_Id == TOK_WHILE );
		UTEST_ASSERT( tokens[ 18 ].m_Id == TOK_PAREN_LEFT );
		UTEST_ASSERT( tokens[ 19 ].m_Id == TOK_TRUE );
		UTEST_ASSERT( tokens[ 20 ].m_Id == TOK_PAREN_RIGHT );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Generate tokens from a sentence (3)" )
	{
		auto tokens = Lexer( "2.222+2.222" );

		UTEST_ASSERT( tokens.size() == 3 );
		UTEST_ASSERT( tokens[ 0 ].m_Id == TOK_DBL );
		UTEST_ASSERT( tokens[ 1 ].m_Id == TOK_PLUS );
		UTEST_ASSERT( tokens[ 2 ].m_Id == TOK_DBL );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Generate tokens from a sentence (4)" )
	{
		auto tokens = Lexer( "a.z[ 2 ].e;" );

		UTEST_ASSERT( tokens.size() == 9 );
		UTEST_ASSERT( tokens[ 0 ].m_Id == TOK_NAME );
		UTEST_ASSERT( tokens[ 1 ].m_Id == TOK_DOT );
		UTEST_ASSERT( tokens[ 2 ].m_Id == TOK_NAME );
		UTEST_ASSERT( tokens[ 3 ].m_Id == TOK_SQUARE_BRACKET_LEFT );
		UTEST_ASSERT( tokens[ 4 ].m_Id == TOK_INT );
		UTEST_ASSERT( tokens[ 5 ].m_Id == TOK_SQUARE_BRACKET_RIGHT );
		UTEST_ASSERT( tokens[ 6 ].m_Id == TOK_DOT );
		UTEST_ASSERT( tokens[ 7 ].m_Id == TOK_NAME );
		UTEST_ASSERT( tokens[ 8 ].m_Id == TOK_SCOLON );

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
		UTEST_ASSERT( symbols[ 1 ].m_Id == TOK_PLUS );
		UTEST_ASSERT( symbols[ 5 ].m_Id == TOK_MINUS );
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
		UTEST_ASSERT( symbols[ 3 ].m_Id == TOK_2EQUALS );
		UTEST_ASSERT( symbols[ 3 ].m_String == "==" );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Debug info generation" )
	{
		auto symbols = Lexer( "5 \n+ 2 != 3\n * 2+2\n*1;" );

		UTEST_ASSERT( symbols.size() == 12 );
		UTEST_ASSERT( symbols[ 0 ].m_ColNr == 0 );
		UTEST_ASSERT( symbols[ 0 ].m_LineNr == 1 );
		UTEST_ASSERT( symbols[ 1 ].m_ColNr == 0 );
		UTEST_ASSERT( symbols[ 1 ].m_LineNr == 2 );
		UTEST_ASSERT( symbols[ 2 ].m_ColNr == 2 );
		UTEST_ASSERT( symbols[ 2 ].m_LineNr == 2 );
		UTEST_ASSERT( symbols[ 3 ].m_ColNr == 4 );
		UTEST_ASSERT( symbols[ 3 ].m_LineNr == 2 );
		UTEST_ASSERT( symbols[ 6 ].m_ColNr == 3 );
		UTEST_ASSERT( symbols[ 6 ].m_LineNr == 3 );
		UTEST_ASSERT( symbols[ 7 ].m_ColNr == 4 ); // +
		UTEST_ASSERT( symbols[ 7 ].m_LineNr == 3 );
		UTEST_ASSERT( symbols[ 8 ].m_ColNr == 5 ); // 2
		UTEST_ASSERT( symbols[ 8 ].m_LineNr == 3 );
		UTEST_ASSERT( symbols[ 9 ].m_ColNr == 0 ); // *
		UTEST_ASSERT( symbols[ 9 ].m_LineNr == 4 );
		UTEST_ASSERT( symbols[ 10 ].m_ColNr == 1 ); // 1
		UTEST_ASSERT( symbols[ 10 ].m_LineNr == 4 );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "String constants" )
	{
		auto tokens = Lexer( "a + \"this is a /*string*/ constant // comments dont apply\";" );

		UTEST_ASSERT( tokens.size() == 4 );
		UTEST_ASSERT( tokens[ 0 ].m_Id == TOK_NAME );
		UTEST_ASSERT( tokens[ 1 ].m_Id == TOK_PLUS );
		UTEST_ASSERT( tokens[ 2 ].m_Id == TOK_STR );
		UTEST_ASSERT( tokens[ 2 ].m_String == "this is a /*string*/ constant // comments dont apply" );
		UTEST_ASSERT( tokens[ 3 ].m_Id == TOK_SCOLON );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Empty strings" )
	{
		auto tokens = Lexer( "\"\";" );

		UTEST_ASSERT( tokens.size() == 2 );
		UTEST_ASSERT( tokens[ 0 ].m_Id == TOK_STR );
		UTEST_ASSERT( tokens[ 1 ].m_Id == TOK_SCOLON );
		UTEST_ASSERT( tokens[ 0 ].m_String == "" );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_CASE( "Word separation" )
	{
		auto tokens = Lexer( "const whileHalf = 0;" );

		UTEST_ASSERT( tokens.size() == 5 );
		UTEST_ASSERT( tokens[ 0 ].m_Id == TOK_CONST );
		UTEST_ASSERT( tokens[ 1 ].m_Id == TOK_NAME );
		UTEST_ASSERT( tokens[ 2 ].m_Id == TOK_EQUALS );
		UTEST_ASSERT( tokens[ 3 ].m_Id == TOK_INT );
		UTEST_ASSERT( tokens[ 4 ].m_Id == TOK_SCOLON );

		tokens = Lexer( "const halfwhile = 0;" );

		UTEST_ASSERT( tokens.size() == 5 );
		UTEST_ASSERT( tokens[ 0 ].m_Id == TOK_CONST );
		UTEST_ASSERT( tokens[ 1 ].m_Id == TOK_NAME );
		UTEST_ASSERT( tokens[ 2 ].m_Id == TOK_EQUALS );
		UTEST_ASSERT( tokens[ 3 ].m_Id == TOK_INT );
		UTEST_ASSERT( tokens[ 4 ].m_Id == TOK_SCOLON );

		tokens = Lexer( "const hwhileh = 0;" );

		UTEST_ASSERT( tokens.size() == 5 );
		UTEST_ASSERT( tokens[ 0 ].m_Id == TOK_CONST );
		UTEST_ASSERT( tokens[ 1 ].m_Id == TOK_NAME );
		UTEST_ASSERT( tokens[ 2 ].m_Id == TOK_EQUALS );
		UTEST_ASSERT( tokens[ 3 ].m_Id == TOK_INT );
		UTEST_ASSERT( tokens[ 4 ].m_Id == TOK_SCOLON );

		UTEST_CASE_CLOSED();
	}( );

	UTEST_END();
}
