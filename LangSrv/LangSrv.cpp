#include "QLibPCH.h"

#include <sstream>
#include <iomanip>

#include "../Library/Common/Chunk.h"
#include "../Library/Compiler/Compiler.h"
#include "../Library/Common/Disassembler.h"
#include "../Library/STL/NativeModule.h"

bool g_ListSymbols = false;
bool g_FirstSymbol = true;
std::string g_SymbolList;
std::string g_DisasmList;
std::unordered_map< const QScript::Chunk_t*, int > g_Disassemblies;

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

std::string ReadFile( const std::string& path )
{
	std::string content = "";
	std::ifstream programFile;

	programFile.open( path );

	if ( programFile.is_open() )
	{
		std::string line;
		while ( std::getline( programFile, line ) )
			content += line + "\n";

		programFile.close();
	}
	else
	{
		throw Exception( "langsrv_no_file_found", "File \"" + path + "\" was not found" );
	}

	return content;
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

int AddDisassembly( const QScript::Chunk_t* chunk, const std::string& name )
{
	auto disasm = Disassembler::DisassembleChunk( *chunk, name );

	if ( g_DisasmList.length() == 0 )
		g_DisasmList = "{";
	else
		g_DisasmList += ",{";

	g_DisasmList += "\"name\":\"" + disasm.m_Name + "\",\"opcodes\":[";

	for ( size_t i = 0; i < disasm.m_Opcodes.size(); ++i )
	{
		auto opcode = disasm.m_Opcodes[ i ];

		std::string opCodeType = "SIMPLE";
		switch ( opcode.m_Type )
		{
		case Disassembler::OPCODE_LONG: opCodeType = "LONG"; break;
		case Disassembler::OPCODE_SHORT: opCodeType = "SHORT"; break;
		default: break;
		}

		if ( i > 0 ) g_DisasmList += ",";

		g_DisasmList += "{\"full\":\"" + JsonEscape( opcode.m_Full ) + "\""
			+ ",\"address\":\"" + std::to_string( opcode.m_Address ) + "\""
			+ ",\"lineNr\":" + std::to_string( opcode.m_LineNr )
			+ ",\"colNr\":" + std::to_string( opcode.m_ColNr )
			+ ",\"token\":\"" + JsonEscape( opcode.m_Token ) + "\""
			+ ",\"type\":\"" + opCodeType + "\""
			+ ",\"size\":\"" + std::to_string( opcode.m_Size ) + "\""
			+ "}";
	}

	int disasmId = g_Disassemblies.size() + 1;

	g_DisasmList += "], \"id\": " + std::to_string( disasmId ) + "}";

	g_Disassemblies[ chunk ] = disasmId;
	return disasmId;
}

int main( int argc, char* argv[] )
{
#ifdef QS_MEMLEAK_TEST
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	std::string next;
	std::string input = "";

	g_ListSymbols = GetArg( "--listSymbols", argc, argv, &next );

	if ( GetArg( "--file", argc, argv, &next ) )
		input = ReadFile( next );

	for ( ;;)
	{
		try
		{
			std::string source;
			std::string newLine;

			if ( input.length() == 0 )
			{
				while ( std::getline( std::cin, newLine ) )
					source += newLine + "\n";
			}
			else
			{
				source = input;
			}

			QScript::Config_t config( true );

			config.m_IdentifierCb = []( int lineNr, int colNr, Compiler::Variable_t& variable, const std::string& context ) -> void {
				if ( !g_ListSymbols )
					return;

				std::string type = Compiler::TypeToString( variable.m_Type );
				std::string returnType = Compiler::TypeToString( variable.m_ReturnType );

				if ( !g_FirstSymbol )
					g_SymbolList += ",";

				// Regular symbol
				g_SymbolList += "{\"context\":\"" + context + "\""
					+ ",\"lineNr\":" + std::to_string( lineNr )
					+ ",\"colNr\":" + std::to_string( colNr )
					+ ",\"name\":\"" + JsonEscape( variable.m_Name ) + "\""
					+ ",\"isConst\": " + ( variable.m_IsConst ? "true" : "false" )
					+ ",\"type\":\"" + JsonEscape( type ) + "\""
					+ ",\"returnType\":\"" + JsonEscape( returnType ) + "\"";

				if ( variable.m_IsConst && variable.m_Function )
				{
					auto function = variable.m_Function;

					// Function constant, include disassembly
					auto chunk = function->GetChunk();

					auto disassembly = g_Disassemblies.find( chunk );

					int disasmId = 0;
					if ( disassembly == g_Disassemblies.end() )
						disasmId = AddDisassembly( chunk, function->GetName() );
					else
						disasmId = disassembly->second;

					g_SymbolList += ",\"disassemblyId\":" + std::to_string( disasmId ) + ",";

					g_SymbolList += "\"args\":[";

					auto& argList = function->GetArgs();
					for ( size_t i = 0; i < argList.size(); ++i )
					{
						auto arg = argList[ i ];

						std::string argType = Compiler::TypeToString( arg.m_Type );
						g_SymbolList += "{\"name\":\"" + JsonEscape( arg.m_Name ) + "\",\"type\":\"" + argType + "\"}";

						if ( i < argList.size() - 1 )
							g_SymbolList += ",";
					}

					g_SymbolList += "]";
				}

				g_SymbolList += "}";

				g_FirstSymbol = false;
			};

			config.m_ImportCb = []( int lineNr, int colNr, const std::string& moduleName, const std::vector< QScript::NativeFunctionSpec_t >& functions ) -> void {
				if ( !g_ListSymbols )
					return;

				if ( !g_FirstSymbol )
					g_SymbolList += ",";

				g_SymbolList += std::string( "{\"context\":\"Import\"" )
					+ ",\"lineNr\":" + std::to_string( lineNr )
					+ ",\"colNr\":" + std::to_string( colNr )
					+ ",\"name\": \"" + JsonEscape( moduleName ) + "\""
					+ ",\"isConst\": true"
					+ ",\"type\":\"module\""
					+ ",\"moduleFunctions\":[";

				for ( size_t i = 0; i < functions.size(); ++i )
				{
					auto& func = functions[ i ];
					std::string returnType = Compiler::TypeToString( func.m_ReturnTypeBits );

					g_SymbolList += std::string( "{" )
						+ "\"name\":\"" + JsonEscape( func.m_Name ) + "\","
						+ "\"returnType\":\"" + JsonEscape( returnType ) + "\","
						+ "\"args\":[";

					for ( size_t j = 0; j < func.m_Args.size(); ++j )
					{
						auto& arg = func.m_Args[ j ];

						std::string argType = Compiler::TypeToString( arg.m_TypeBits );
						std::string argReturnType = Compiler::TypeToString( arg.m_ReturnTypeBits );

						g_SymbolList += std::string( "{" )
							+ "\"name\":\"" + JsonEscape( arg.m_ArgName ) + "\","
							+ "\"type\":\"" + JsonEscape( argType ) + "\","
							+ "\"returnType\":\"" + JsonEscape( argReturnType ) + "\","
							+ "\"isVarArgs\":" + ( arg.m_IsVarArgs ? "true" : "false" )
							+ "}" + ( j < func.m_Args.size() - 1 ? "," : "" );
					}

					g_SymbolList += std::string( "]}" ) + ( i < functions.size() - 1 ? "," : "" );
				}

				g_SymbolList += "]}";
				g_FirstSymbol = false;
			};

			auto mainFunction = QScript::Compile( source, config );

			std::cout <<
				"{\"status\":\"OK\",\"errors\":[],\"symbols\":["
				+ g_SymbolList
				+ "],\"disasm\":["
				+ g_DisasmList
				+ "]}" << std::endl;

			QScript::FreeFunction( mainFunction );
		}
		catch ( std::vector< CompilerException >& exceptions )
		{
			std::cout << "{ \"status\": \"failed\", \"errors\": [";

			for ( size_t i = 0; i < exceptions.size(); ++i )
			{
				auto& ex = exceptions[ i ];

				auto description = ( ex.What() + std::string( " (line " )
					+ std::to_string( ex.LineNr() )
					+ " character " + std::to_string( ex.ColNr() )
					+ " token \"" + ex.Token() + "\")" );

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
			std::cout << "{ \"status\": \"failed\", \"errors\": [";
			std::cout << "{ \"id\": \"" + exception.id() + "\", \"desc\": \"" + exception.describe() + "\", \"generic\": true }] }" << std::endl;
		}
		catch ( const std::exception& exception )
		{
			std::cout << "{ \"status\": \"failed\", \"errors\": [";
			std::cout << "{ \"id\": \"unknown\", \"desc\": \"" + std::string( exception.what() ) + "\", \"generic\": true }] }" << std::endl;
		}
		catch ( ... )
		{
			std::cout << "{ \"status\": \"failed\", \"errors\": [";
			std::cout << "{ \"id\": \"unknown\", \"generic\": true }] }" << std::endl;
		}

		if ( GetArg( "--exit", argc, argv, &next ) )
			break;
	}
	return 0;
}
