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

	const std::map<Token, KeywordInfo_t> LanguageSymbols = {
		{ TOK_PLUS,			{ TOK_PLUS, 		"+", 		BP_ARITHMETIC_ADDSUB,		false } },
		{ TOK_MINUS,		{ TOK_MINUS, 		"-", 		BP_ARITHMETIC_ADDSUB,		false } },
		{ TOK_STAR,			{ TOK_STAR, 		"*", 		BP_ARITHMETIC_MULDIV,		false } },
		{ TOK_SLASH,		{ TOK_SLASH, 		"/", 		BP_ARITHMETIC_MULDIV,		false } },
		{ TOK_PERCENT,		{ TOK_PERCENT, 		"%",		BP_ARITHMETIC_POWMOD,		false } },
		{ TOK_2STAR,		{ TOK_2STAR, 		"**",		BP_ARITHMETIC_POWMOD,		false } },

		{ TOK_EQUALS,		{ TOK_EQUALS,		"=",		BP_ASSIGN,					false } },
		{ TOK_EQUALSADD,	{ TOK_EQUALSADD, 	"+=",		BP_ASSIGN,					false } },
		{ TOK_EQUALSSUB,	{ TOK_EQUALSSUB, 	"-=",		BP_ASSIGN,					false } },
		{ TOK_EQUALSMUL,	{ TOK_EQUALSMUL, 	"*=",		BP_ASSIGN,					false } },
		{ TOK_EQUALSDIV,	{ TOK_EQUALSDIV, 	"/=",		BP_ASSIGN,					false } },
		{ TOK_EQUALSMOD,	{ TOK_EQUALSMOD, 	"%=",		BP_ASSIGN,					false } },

		{ TOK_2EQUALS, 		{ TOK_2EQUALS, 		"==", 		BP_EQUALITY, 				false } },
		{ TOK_NOTEQUALS, 	{ TOK_NOTEQUALS, 	"!=", 		BP_EQUALITY, 				false } },
		{ TOK_GREATERTHAN, 	{ TOK_GREATERTHAN, 	">", 		BP_EQUALITY, 				false } },
		{ TOK_GREATEREQUAL,	{ TOK_GREATEREQUAL, ">=", 		BP_EQUALITY, 				false } },
		{ TOK_LESSTHAN, 	{ TOK_LESSTHAN, 	"<", 		BP_EQUALITY, 				false } },
		{ TOK_LESSEQUAL,	{ TOK_LESSEQUAL, 	"<=", 		BP_EQUALITY, 				false } },

		{ TOK_AND,			{ TOK_AND, 			"&&", 		BP_LOGIC, 					false } },
		{ TOK_OR,			{ TOK_OR, 			"||", 		BP_LOGIC, 					false } },
		{ TOK_BANG, 		{ TOK_BANG, 		"!", 		BP_LOGIC_NOT,				false } },

		{ TOK_PAREN_LEFT,	{ TOK_PAREN_LEFT, 	"(", 		BP_OPENPAREN,				false } },
		{ TOK_PAREN_RIGHT,	{ TOK_PAREN_RIGHT, 	")", 		BP_NONE,					false } },

		{ TOK_SCOLON,		{ TOK_SCOLON, 		";", 		BP_NONE,					false } },
		{ TOK_BRACE_LEFT, 	{ TOK_BRACE_LEFT, 	"{", 		BP_NONE,					false } },
		{ TOK_BRACE_RIGHT, 	{ TOK_BRACE_RIGHT, 	"}", 		BP_NONE,					false } },
		{ TOK_COMMA, 		{ TOK_COMMA, 		",", 		BP_NONE,					false } },
		{ TOK_DOT, 			{ TOK_DOT, 			".", 		BP_DOT,						false } },

		{ TOK_2PLUS,		{ TOK_2PLUS, 		"++",		BP_INCDEC,					false } },
		{ TOK_2MINUS,		{ TOK_2MINUS, 		"--",		BP_INCDEC,					false } },
		{ TOK_ARROW,		{ TOK_ARROW, 		"->", 		BP_NONE,					false } },

		{ TOK_CLASS, 		{ TOK_CLASS, 		"class", 	BP_NONE,					true } },
		{ TOK_DO,			{ TOK_DO,			"do",		BP_NONE,					true } },
		{ TOK_ELSE,			{ TOK_ELSE,			"else",		BP_NONE,					true } },
		{ TOK_FALSE,		{ TOK_FALSE, 		"false", 	BP_NONE,					true } },
		{ TOK_FOR,			{ TOK_FOR, 			"for", 		BP_NONE,					true } },
		{ TOK_IF,			{ TOK_IF,			"if",		BP_NONE,					true } },
		{ TOK_IMPORT,		{ TOK_IMPORT,		"import",	BP_NONE,					true } },
		{ TOK_NULL,			{ TOK_NULL, 		"null", 	BP_NONE,					true } },
		{ TOK_RETURN,		{ TOK_RETURN, 		"return", 	BP_NONE,					true } },
		{ TOK_TRUE,			{ TOK_TRUE, 		"true", 	BP_NONE,					true } },
		{ TOK_VAR, 			{ TOK_VAR, 			"var", 		BP_VAR,						true } },
		{ TOK_CONST, 		{ TOK_CONST, 		"const", 	BP_VAR, 					true } },
		{ TOK_WHILE, 		{ TOK_WHILE, 		"while", 	BP_NONE,					true } },
	};

	std::vector< Token_t > Lexer( const std::string& source )
	{
		enum LexerState
		{
			LS_CODE,
			LS_STRING,
			LS_COMMENT,
			LS_COMMENT_MULTILINE,
		};

		std::regex intRegEx( "[-+]?([0-9]*[0-9]+|[0-9]+)" );
		std::regex decimalRegEx( "[-+]?\\d+\\.\\d*" );

		std::vector< Token_t > results;
		std::vector< KeywordInfo_t > languageSymbols;

		std::string stringBuffer = "";

		LexerState lexerState = LS_CODE;
		size_t currentLineNr = 0;
		size_t currentColNr = 0;

		// Populate language keywords array
		for ( auto it = LanguageSymbols.begin(); it != LanguageSymbols.end(); ++it )
			languageSymbols.push_back( it->second );

		// Sort the language keywords to have longest character words at the top
		std::sort( languageSymbols.begin(), languageSymbols.end(), []( KeywordInfo_t a, KeywordInfo_t b ) -> bool
		{
			return a.m_String.length() > b.m_String.length();
		} );

		// Is this string an integer ?
		auto isInteger = [ &intRegEx ]( const std::string& token ) -> bool {
			return std::regex_match( token, intRegEx );
		};

		// Is it a decimal ?
		auto isDecimal = [ &decimalRegEx ]( const std::string& token ) -> bool {
			return std::regex_match( token, decimalRegEx );
		};

		// Get the longest searchable token length
		size_t longestToken = languageSymbols.size() > 0 ? languageSymbols[ 0 ].m_String.length() : 0;

		auto pushToken = [ &currentLineNr, &currentColNr, &stringBuffer, &results, &isInteger, &isDecimal ]( bool isStrCnst = false ) -> void {
			if ( stringBuffer.length() == 0 && !isStrCnst )
				return;

			Token token = Token::TOK_NAME;

			if ( isStrCnst )
			{
				token = Token::TOK_STR;
			}
			else
			{
				if ( isDecimal( stringBuffer ) )
					token = Token::TOK_DBL;
				else if ( isInteger( stringBuffer ) )
					token = Token::TOK_INT;
			}

			// Push the last accumulated string and flush stringBuffer
			results.push_back( Token_t {
				token,
				BindingPower::BP_NONE,
				( int ) currentLineNr,
				( int ) currentColNr,
				stringBuffer,
			} );

			currentColNr += stringBuffer.length();
			stringBuffer = "";
		};

		auto pushKeyword = [ &languageSymbols, &results, &currentLineNr,
			&currentColNr, &pushToken, &stringBuffer, &isInteger ]( std::string pattern ) -> int
		{
			// Iterate all the commons
			for ( auto symIt = languageSymbols.cbegin(); symIt != languageSymbols.cend(); ++symIt )
			{
				if ( pattern.compare( symIt->m_IsWord ? 0 : stringBuffer.length(), symIt->m_String.length(), symIt->m_String ) == 0 )
				{
					if ( symIt->m_String == "." && isInteger( stringBuffer ) )
						return 0;

					pushToken();

					// Push the found keyword
					results.push_back( Token_t {
						symIt->m_Token,
						symIt->m_LBP,
						( int ) currentLineNr,
						( int ) currentColNr,
						symIt->m_String,
					} );

					currentColNr += symIt->m_String.length();
					return ( int ) symIt->m_String.length();
				}
			}

			return 0;
		};

		// Iterate through source and look for tokens
		for ( auto srcIt = source.cbegin(); srcIt != source.cend(); ++srcIt )
		{
			switch ( lexerState )
			{
			case LS_CODE:
			{
				auto cursor = std::distance( source.cbegin(), srcIt );
				auto nextTwo = source.substr( cursor, 2 );

				if ( nextTwo == "//" )
				{
					// Making a comment
					lexerState = LS_COMMENT;
					break;
				}
				else if ( nextTwo == "/*" )
				{
					// Making a comment
					lexerState = LS_COMMENT_MULTILINE;
					break;
				}
				else if ( *srcIt == '"' )
				{
					// Switch to string parsing
					lexerState = LS_STRING;
					break;
				}

				int keywordLength = pushKeyword( source.substr( std::distance( source.cbegin(), srcIt - stringBuffer.length() ),
					longestToken + stringBuffer.length() ) );

				if ( keywordLength > 0 )
				{
					// Increment the iterator by the amount of characters
					srcIt += keywordLength - 1;
					continue;
				}

				switch ( *srcIt )
				{
				case '\n':
				case '\t':
				case ' ':
				{
					pushToken();
					if ( *srcIt != '\n' )
					{
						++currentColNr;
					}
					else
					{
						currentColNr = 0;
						++currentLineNr;
					}

					break;
				}
				default:
				{
					// Add the character to the symbol buffer
					stringBuffer += *srcIt;
					break;
				}
				}

				break;
			}
			case LS_STRING:
			{
				if ( *srcIt == '"' )
				{
					// Switch back to code
					lexerState = LS_CODE;

					// Append string constant
					pushToken( true );
					break;
				}

				// Append the character to the string
				stringBuffer += *srcIt;
				break;
			}
			case LS_COMMENT:
			{
				if ( *srcIt == '\n' )
					lexerState = LS_CODE;

				// During a normal comment, just ignore everything
				stringBuffer = "";
				break;
			}
			case LS_COMMENT_MULTILINE:
			{
				if ( stringBuffer.length() > 0 )
				{
					// Look for comment terminating sequence
					if ( ( stringBuffer.substr( stringBuffer.length() - 1, 1 ) + *srcIt ) == "*/" )
					{
						lexerState = LS_CODE;
						stringBuffer = "";
						break;
					}
				}

				stringBuffer += *srcIt;
				break;
			}
			}
		}

		pushToken();
		return results;
	}
}
