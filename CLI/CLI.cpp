#include "QLibPCH.h"
#include "../Library/Common/Chunk.h"
#include "../Library/Compiler/Compiler.h"
#include "../Library/Compiler/AST/AST.h"

#define EXCEPTION_HANDLING \
catch ( std::vector< CompilerException >& exceptions ) \
{ \
	for ( auto ex : exceptions ) \
		std::cout << ex.describe() << std::endl; \
} \
catch ( const RuntimeException& exception ) \
{ \
	if ( exception.id() != "rt_exit" ) \
		std::cout << "Exception (" << exception.id() << "): " << exception.describe() << std::endl; \
} \
catch ( const Exception& exception ) \
{ \
	std::cout << "Exception (" << exception.id() << "): " << exception.describe() << std::endl; \
} \
catch ( const std::exception& exception ) \
{ \
	std::cout << "Exception: " << exception.what() << std::endl; \
} \
catch ( ... ) \
{ \
	std::cout << "Unknown exception occurred." << std::endl; \
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
			content += line;

		programFile.close();
	}
	else
	{
		throw Exception( "cli_no_file_found", "File \"" + path + "\" was not found" );
	}

	return content;
}

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

int main( int argc, char* argv[] )
{
#ifdef QS_MEMLEAK_TEST
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	std::string next;
	std::string input = "";

	if ( GetArg( "--file", argc, argv, &next ) )
		input = ReadFile( next );

	if ( GetArg( "--repl", argc, argv, &next ) )
	{
		QScript::Repl();
	}
	else if ( GetArg( "--typer", argc, argv, &next ) )
	{
		for (;;)
		{
			try
			{
				std::string source = input;

				if ( source.length() == 0 )
				{
					std::cout << "Typer >";
					std::getline( std::cin, source );
				}

				auto exprTypes = QScript::Typer( source );

				for ( auto type : exprTypes )
				{
					if ( type.second != 1024 /* TYPE_NONE */ )
					{
						std::cout << Compiler::TypeToString( type.first )
							<< " -> " << Compiler::TypeToString( type.second ) << std::endl;
					}
					else
					{
						std::cout << Compiler::TypeToString( type.first ) << std::endl;
					}
				}
			}
			EXCEPTION_HANDLING;

			if ( input.length() > 0 )
				break;
		}
	}
	else if ( GetArg( "--json", argc, argv, &next ) )
	{
		for (;;)
		{
			try
			{
				std::string source = input;

				if ( source.length() == 0 )
				{
					std::cout << "Json >";
					std::getline( std::cin, source );
				}

				auto astNodes = QScript::GenerateAST( source );

				for ( auto node : astNodes )
					std::cout << node->ToJson() << std::endl;

				for ( auto node : astNodes )
				{
					node->Release();
					delete node;
				}

				// NOTE: Currently leaks memory for values nested in astNodes tree
			}
			EXCEPTION_HANDLING;

			if ( input.length() > 0 )
				break;
		}
	}
	else if ( input.length() > 0 )
	{
		QScript::FunctionObject* function = NULL;

		try
		{
			function = QScript::Compile( ReadFile( next ) );
			QScript::Interpret( *function );
		}
		EXCEPTION_HANDLING;

		QScript::FreeFunction( function );
	}

	return 0;
}
