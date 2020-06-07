#include "QLibPCH.h"

#include <sstream>
#include <iomanip>

#include "..\Library\Common\Chunk.h"
#include "..\Library\Compiler\Compiler.h"

bool g_ListSymbols = false;
bool g_FirstSymbol = true;
std::string g_SymbolList;

bool GetArg( const std::string& argument, int argc, char* argv[], std::string* next )
{
	for ( int i = 1; i < argc; ++i )
	{
		if ( argv[ i ] == argument )
		{
			if ( i + 1 < argc )
				*next = argv[ i + 1 ];
			else
				*next = "";

			return true;
		}
	}

	return false;
}

std::string JsonEscape( const std::string& input )
{
	std::ostringstream o;
	for ( auto c = input.cbegin(); c != input.cend(); c++ )
	{
		switch ( *c )
		{
		case '"': o << "\\\""; break;
		case '\\': o << "\\\\"; break;
		case '\b': o << "\\b"; break;
		case '\f': o << "\\f"; break;
		case '\n': o << "\\n"; break;
		case '\r': o << "\\r"; break;
		case '\t': o << "\\t"; break;
		default:
		{
			if ( '\x00' <= *c && *c <= '\x1f' ) {
				o << "\\u"
					<< std::hex << std::setw( 4 ) << std::setfill( '0' ) << ( int ) *c;
			}
			else {
				o << *c;
			}
		}
		}
	}

	return o.str();
}

int main( int argc, char* argv[] )
{
#ifdef QS_MEMLEAK_TEST
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	std::string next;
	g_ListSymbols = GetArg( "--listSymbols", argc, argv, &next );

	for ( ;;)
	{
		try
		{
			std::string source;
			std::string newLine;

			while ( std::getline( std::cin, newLine ) )
				source += newLine + "\n";

			QScript::Config_t config( true );

			config.m_IdentifierCb = []( int lineNr, int colNr, Compiler::Variable_t& variable, const std::string& context ) -> void {
				if ( !g_ListSymbols )
					return;

				std::string type = Compiler::TypeToString( variable.m_Type );
				std::string returnType = Compiler::TypeToString( variable.m_ReturnType );

				if ( !g_FirstSymbol )
					g_SymbolList += ",";

				g_SymbolList += "{ \"context\": \"" + context + "\""
					+ ", \"lineNr\": " + std::to_string( lineNr )
					+ ", \"colNr\": " + std::to_string( colNr )
					+ ", \"name\": \"" + JsonEscape( variable.m_Name ) + "\""
					+ ", \"isConst\": " + ( variable.m_IsConst ? "true" : "false" )
					+ ", \"type\": \"" + JsonEscape( type ) + "\""
					+ ", \"returnType\": \"" + JsonEscape( returnType ) + "\""
					+ "}";

				g_FirstSymbol = false;
			};

			auto mainFunction = QScript::Compile( source, config );

			std::cout <<
				"{ \"status\": \"OK\", \"errors\": [], \"symbols\": ["
				+ g_SymbolList
				+ "]}" << std::endl;

			QScript::FreeFunction( mainFunction );
		}
		catch ( std::vector< CompilerException >& exceptions )
		{
			std::cout << "{ \"status\": \"failed\", \"errors\": [";

			for ( size_t i = 0; i < exceptions.size(); ++i )
			{
				auto& ex = exceptions[ i ];

				auto description = ( ex.What() + std::string( " on line " )
					+ std::to_string( ex.LineNr() ) + " character " + std::to_string( ex.ColNr() ) );

				std::cout << std::string( "{" )
					+ "\"lineNr\": " + std::to_string( ex.LineNr() ) + ","
					+ "\"colNr\": " + std::to_string( ex.ColNr() ) + ","
					+ "\"token\": \"" + JsonEscape( ex.Token() ) + "\","
					+ "\"desc\": \"" + JsonEscape( description ) + "\""
					+ "}" + ( i < exceptions.size() - 1 ? "," : "" );
			}

			std::cout <<
				"], \"symbols\": ["
				+ g_SymbolList
				+ "]}" << std::endl;
		}
		catch ( const Exception& exception )
		{
			std::cout << "[" + exception.id() + "]:" + exception.describe() << std::endl;
		}
		catch ( const std::exception& exception )
		{
			std::cout << std::string( "[std_generic]:" ) + exception.what() << std::endl;
		}
		catch ( ... )
		{
			std::cout << "[unknown]" << std::endl;
		}

		if ( GetArg( "--exit", argc, argv, &next ) )
			break;
	}
	return 0;
}
