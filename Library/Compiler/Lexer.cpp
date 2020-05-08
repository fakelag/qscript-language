#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Compiler.h"

namespace Compiler
{
	struct KeywordInfo_t
	{
		Token				m_Token;
		std::string			m_String;
		int					m_LBP;
		bool				m_IsWord;
	};

	const std::map<Token, KeywordInfo_t> LanguageSymbols ={
		{ TOK_PLUS,					{ TOK_PLUS, 					"+", 		BP_ARITHMETIC_ADDSUB,		false } },
		{ TOK_MINUS,				{ TOK_MINUS, 					"-", 		BP_ARITHMETIC_ADDSUB,		false } },
		{ TOK_STAR,					{ TOK_STAR, 					"*", 		BP_ARITHMETIC_MULDIV,		false } },
		{ TOK_SLASH,				{ TOK_SLASH, 					"/", 		BP_ARITHMETIC_MULDIV,		false } },
		{ TOK_PERCENT,				{ TOK_PERCENT, 					"%",		BP_ARITHMETIC_POWMOD,		false } },
		{ TOK_2STAR,				{ TOK_2STAR, 					"**",		BP_ARITHMETIC_POWMOD,		false } },

		{ TOK_EQUALS,				{ TOK_EQUALS,					"=",		BP_ASSIGN,					false } },
		{ TOK_EQUALSADD,			{ TOK_EQUALSADD, 				"+=",		BP_ASSIGN,					false } },
		{ TOK_EQUALSSUB,			{ TOK_EQUALSSUB, 				"-=",		BP_ASSIGN,					false } },
		{ TOK_EQUALSMUL,			{ TOK_EQUALSMUL, 				"*=",		BP_ASSIGN,					false } },
		{ TOK_EQUALSDIV,			{ TOK_EQUALSDIV, 				"/=",		BP_ASSIGN,					false } },
		{ TOK_EQUALSMOD,			{ TOK_EQUALSMOD, 				"%=",		BP_ASSIGN,					false } },

		{ TOK_2EQUALS, 				{ TOK_2EQUALS, 					"==", 		BP_EQUALITY, 				false } },
		{ TOK_NOTEQUALS, 			{ TOK_NOTEQUALS, 				"!=", 		BP_EQUALITY, 				false } },
		{ TOK_GREATERTHAN, 			{ TOK_GREATERTHAN, 				">", 		BP_EQUALITY, 				false } },
		{ TOK_GREATEREQUAL,			{ TOK_GREATEREQUAL,				">=", 		BP_EQUALITY, 				false } },
		{ TOK_LESSTHAN, 			{ TOK_LESSTHAN, 				"<", 		BP_EQUALITY, 				false } },
		{ TOK_LESSEQUAL,			{ TOK_LESSEQUAL, 				"<=", 		BP_EQUALITY, 				false } },

		{ TOK_AND,					{ TOK_AND, 						"&&", 		BP_LOGIC, 					false } },
		{ TOK_OR,					{ TOK_OR, 						"||", 		BP_LOGIC, 					false } },
		{ TOK_BANG, 				{ TOK_BANG, 					"!", 		BP_LOGIC_NOT,				false } },

		{ TOK_SQUARE_BRACKET_LEFT,  { TOK_SQUARE_BRACKET_LEFT,		"[",		BP_OPENSQUARE_BRACKET,		false } },
		{ TOK_SQUARE_BRACKET_RIGHT, { TOK_SQUARE_BRACKET_RIGHT,		"]",		BP_NONE,					false } },

		{ TOK_PAREN_LEFT,			{ TOK_PAREN_LEFT, 				"(", 		BP_OPENPAREN,				false } },
		{ TOK_PAREN_RIGHT,			{ TOK_PAREN_RIGHT, 				")", 		BP_NONE,					false } },

		{ TOK_SCOLON,				{ TOK_SCOLON, 					";", 		BP_NONE,					false } },
		{ TOK_BRACE_LEFT, 			{ TOK_BRACE_LEFT, 				"{", 		BP_NONE,					false } },
		{ TOK_BRACE_RIGHT, 			{ TOK_BRACE_RIGHT, 				"}", 		BP_NONE,					false } },
		{ TOK_COMMA, 				{ TOK_COMMA, 					",", 		BP_NONE,					false } },
		{ TOK_DOT, 					{ TOK_DOT, 						".", 		BP_DOT,						false } },
		{ TOK_QUERY, 				{ TOK_QUERY, 					"?", 		BP_INLINE_IF,				false } },
		{ TOK_COLON, 				{ TOK_COLON, 					":", 		BP_INLINE_IF,				false } },

		{ TOK_2PLUS,				{ TOK_2PLUS, 					"++",		BP_INCDEC,					false } },
		{ TOK_2MINUS,				{ TOK_2MINUS, 					"--",		BP_INCDEC,					false } },
		{ TOK_ARROW,				{ TOK_ARROW, 					"->", 		BP_NONE,					false } },

		{ TOK_ARRAY, 				{ TOK_ARRAY, 					"Array", 	BP_NONE,					true } },
		{ TOK_AUTO, 				{ TOK_AUTO, 					"auto", 	BP_VAR,						true } },
		{ TOK_BOOL, 				{ TOK_BOOL, 					"bool", 	BP_VAR,						true } },
		{ TOK_CONST, 				{ TOK_CONST, 					"const", 	BP_VAR, 					true } },
		{ TOK_DO,					{ TOK_DO,						"do",		BP_NONE,					true } },
		{ TOK_ELSE,					{ TOK_ELSE,						"else",		BP_NONE,					true } },
		{ TOK_FALSE,				{ TOK_FALSE, 					"false", 	BP_NONE,					true } },
		{ TOK_FOR,					{ TOK_FOR, 						"for", 		BP_NONE,					true } },
		{ TOK_IF,					{ TOK_IF,						"if",		BP_NONE,					true } },
		{ TOK_IMPORT,				{ TOK_IMPORT,					"import",	BP_NONE,					true } },
		{ TOK_NULL,					{ TOK_NULL, 					"null", 	BP_NONE,					true } },
		{ TOK_NUMBER,				{ TOK_NUMBER, 					"num", 		BP_VAR,						true } },
		{ TOK_RETURN,				{ TOK_RETURN, 					"return", 	BP_NONE,					true } },
		{ TOK_STRING,				{ TOK_STRING, 					"string", 	BP_VAR,						true } },
		{ TOK_TABLE, 				{ TOK_TABLE, 					"Table", 	BP_NONE,					true } },
		{ TOK_TRUE,					{ TOK_TRUE, 					"true", 	BP_NONE,					true } },
		{ TOK_VAR, 					{ TOK_VAR, 						"var", 		BP_VAR,						true } },
		{ TOK_WHILE, 				{ TOK_WHILE, 					"while", 	BP_NONE,					true } },
	};

	std::vector< Token_t > Lexer( const std::string& source )
	{
		static std::vector< KeywordInfo_t > languageOperators;
		static std::vector< KeywordInfo_t > languageWords;

		std::string_view sourceView( source );
		std::vector< Token_t > results;
		std::string backBuffer;

		int lineNumber = 1;
		int columnNumber = 0;

		if ( languageOperators.size() == 0 )
		{
			for ( auto it = LanguageSymbols.begin(); it != LanguageSymbols.end(); ++it )
			{
				if ( it->second.m_IsWord )
					languageWords.push_back( it->second );
				else
					languageOperators.push_back( it->second );
			}

			// Sort the language keywords to have longest character sequence at the top
			std::sort( languageOperators.begin(), languageOperators.end(), []( KeywordInfo_t a, KeywordInfo_t b ) -> bool
			{
				return a.m_String.length() > b.m_String.length();
			} );

			std::sort( languageWords.begin(), languageWords.end(), []( KeywordInfo_t a, KeywordInfo_t b ) -> bool
			{
				return a.m_String.length() > b.m_String.length();
			} );
		}

		auto lookAhead = []( const std::string_view& view, const std::string& word, size_t* out )
		{
			auto pos = view.find( word );

			if ( pos == view.npos )
			{
				*out = pos;
				return false;
			}

			*out = pos;
			return true;
		};

		auto matchItem = []( const std::string_view& input, const std::vector< KeywordInfo_t >& list, KeywordInfo_t* out ) -> bool
		{
			for ( auto keyword : list )
			{
				if ( input.compare( 0, keyword.m_String.length(), keyword.m_String ) == 0 )
				{
					*out = keyword;
					return true;
				}
			}

			return false;
		};

		auto isNumber = []( const std::string_view& input )
		{
			for ( char c : input )
			{
				if ( !isdigit( c ) )
					return false;
			}

			return true;
		};

		auto flushName = [ &backBuffer, &sourceView, &results, &lineNumber, &columnNumber ]()
		{
			if ( backBuffer.length() > 0 )
			{
				Token tokenType = TOK_INT;
				for ( auto c : backBuffer )
				{
					if ( !isdigit( c ) && c != '.' )
						tokenType = TOK_NAME;
					else if ( c == '.' && tokenType == TOK_INT )
						tokenType = TOK_DBL;
				}

				results.push_back( Token_t{ tokenType, 0, lineNumber, columnNumber, backBuffer } );

				columnNumber += backBuffer.length();
				backBuffer = "";
			}
		};

		for ( size_t cursor = 0; cursor < source.length(); ++cursor )
		{
			std::string_view cursorView( source );
			cursorView = cursorView.substr( cursor );

			std::string wordBuffer = backBuffer + cursorView.at( 0 );

			KeywordInfo_t wordInfo, opInfo;
			if ( matchItem( wordBuffer, languageWords, &wordInfo ) )
			{
				/*
					Next needs to either be an operator
					e.g while(...
					or a space
				*/
				if ( wordBuffer[ wordInfo.m_String.length() ] == ' ' )
				{
					results.push_back( Token_t{ wordInfo.m_Token, wordInfo.m_LBP, lineNumber, columnNumber, wordInfo.m_String } );
					backBuffer = "";
					columnNumber += wordInfo.m_String.length();
					continue;
				}
				else if ( matchItem( wordBuffer.substr( wordInfo.m_String.length() ), languageOperators, &opInfo ) )
				{
					/*
						A word followed by an operator
						e.g while(...
					*/
					results.push_back( Token_t{ wordInfo.m_Token, wordInfo.m_LBP, lineNumber, columnNumber, wordInfo.m_String } );
					results.push_back( Token_t{ opInfo.m_Token, opInfo.m_LBP, lineNumber, columnNumber, opInfo.m_String } );

					columnNumber += wordInfo.m_String.length();
					columnNumber += opInfo.m_String.length();

					backBuffer = "";
					continue;
				}
				else
				{
					/*
						This is a part of some other word
						e.g whileVariable = ...
					*/
				}
			}
			else if ( matchItem( cursorView, languageOperators, &opInfo ) )
			{
				switch ( opInfo.m_Token )
				{
				case TOK_DOT:
				{
					if ( isNumber( backBuffer ) )
					{
						std::string_view fractionString = cursorView.substr( 1 );

						size_t i = 0;
						for ( ; i < fractionString.length(); ++i )
						{
							if ( !isdigit( fractionString[ i ] ) )
								break;
						}

						/*
							Numeric values on both sides of "." operator
							e.g: 5.123
							batch together and flush
						*/

						backBuffer += cursorView.substr( 0, i + 1 );
						flushName();

						columnNumber += i + 1;

						cursor += i;
						continue;
					}
					break;
				}
				case TOK_SLASH:
				{
					flushName();

					if ( cursorView.at( 1 ) == '/' )
					{
						// Per-line comment
						size_t nextCursor = 0;
						if ( lookAhead( cursorView.substr( 2 ), "\n", &nextCursor ) )
						{
							cursor += nextCursor;
							++lineNumber;
							columnNumber = 0;
						}
						else
						{
							cursor += cursorView.length();
						}

						cursor += 2;
						continue;
					}
					else if ( cursorView.at( 1 ) == '*' )
					{
						// Multiline comment
						size_t nextCursor = 0;
						if ( lookAhead( cursorView.substr( 2 ), "*/", &nextCursor ) )
							cursor += nextCursor;
						else
							cursor += cursorView.length();

						cursor += 3;
						continue;
					}

					break;
				}
				default:
					break;
				}

				flushName();

				results.push_back( Token_t{ opInfo.m_Token, opInfo.m_LBP, lineNumber, columnNumber, opInfo.m_String } );
				cursor += opInfo.m_String.length() - 1;

				columnNumber += opInfo.m_String.length();

				backBuffer = "";
				continue;
			}

			switch ( cursorView.at( 0 ) )
			{
			case '\"':
			{
				flushName();

				size_t nextCursor = 0;
				if ( lookAhead( cursorView.substr( 1 ), "\"", &nextCursor ) )
				{
					results.push_back( Token_t{ TOK_STR, 0, lineNumber, columnNumber, std::string( cursorView.substr( 1, nextCursor ) ) } );
					columnNumber += nextCursor - cursor + 2;
					cursor += nextCursor + 1;
				}
				else
				{
					cursor += cursorView.length();
				}
				break;
			}
			case ' ':
			{
				flushName();
				++columnNumber;
				break;
			}
			case '\n':
			{
				flushName();
				++lineNumber;
				columnNumber = 0;
				break;
			}
			case '\t':
				break;
			default:
				backBuffer += cursorView.at( 0 );
				break;
			}
		}

		flushName();
		return results;
	}
}
