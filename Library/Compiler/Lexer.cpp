#include "../Includes/QLibPCH.h"
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
		{ TOK_PLUS,			{ TOK_PLUS, "+", 		BP_ARITHMETIC_ADDSUB,		false } },
	};

	std::vector< Token_t > Lexer( const std::string& source )
	{
		std::vector< Token_t > results;
		std::vector<KeywordInfo_t> languageSymbols;

		std::string stringBuffer = "";

		bool parsingString = false;
		int currentLineNr = 0;
		int currentColNr = 0;

		// Populate language keywords array
		for ( auto it = LanguageSymbols.begin(); it != LanguageSymbols.end(); ++it )
			languageSymbols.push_back( it->second );

		// Sort the language keywords to have longest character words at the top
		std::sort( languageSymbols.begin(), languageSymbols.end(), []( KeywordInfo_t a, KeywordInfo_t b ) -> bool
		{
			return a.m_String.length() > b.m_String.length();
		} );

		// Is this string an integer ?
		auto isInteger = []( const std::string token ) -> bool {
			std::regex re("[-+]?([0-9]*[0-9]+|[0-9]+)");
			return std::regex_match( token, re );
		};

		// Is it a decimal ?
		auto isDecimal = []( const std::string token ) -> bool {
			std::regex re("[-+]?\\d+\\.\\d*");
			return std::regex_match( token, re );
		};

		// Get the longest searchable token length
		int longestToken = languageSymbols.size() > 0 ? languageSymbols[ 0 ].m_String.length() : 0;

		auto pushToken = [ &currentLineNr, &currentColNr, &stringBuffer, &results, &isInteger, &isDecimal ]( bool isStrCnst = false ) -> void {
			if ( stringBuffer.length() == 0 )
				return;

			Token token = Token::TOK_NAME;

			if ( isStrCnst )
			{
				// It is a string constant, strip the quotation marks
				stringBuffer = stringBuffer.substr( 1, stringBuffer.length() - 1 );
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
				currentLineNr,
				currentColNr,
				stringBuffer,
			} );

			currentColNr += stringBuffer.length();
			stringBuffer = "";
		};

		auto pushKeyword = [ &languageSymbols, &results, &currentLineNr,
			&currentColNr, &pushToken, &stringBuffer, &isInteger ]( std::string pattern ) -> int {
			// Iterate all the commons
			for ( auto symIt = languageSymbols.cbegin(); symIt != languageSymbols.cend(); ++symIt )
			{
				if ( symIt->m_String == pattern.substr( symIt->m_IsWord ? 0 : stringBuffer.length(), symIt->m_String.length() ) )
				{
					if ( symIt->m_String == "." && isInteger( stringBuffer ) )
						return 0;

					pushToken();

					// Push the found keyword
					results.push_back( Token_t {
						symIt->m_Token,
						symIt->m_LBP,
						currentLineNr,
						currentColNr,
						symIt->m_String,
					} );

					currentColNr += symIt->m_String.length();
					return symIt->m_String.length();
				}
			}

			return 0;
		};

		// Iterate through source and look for tokens
		for ( auto srcIt = source.cbegin(); srcIt != source.cend(); ++srcIt )
		{
			if ( *srcIt == '"' )
			{
				// Switch string parsing state
				parsingString = !parsingString;

				if ( !parsingString )
				{
					// Push the new string constant to the stack
					pushToken( true );
					continue;
				}
			}

			if ( !parsingString )
			{
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
			}
			else
			{
				// Append the character to the string
				stringBuffer += *srcIt;
			}
		}

		pushToken();
		return results;
	}
}
