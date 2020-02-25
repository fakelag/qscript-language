#include "QLibPCH.h"

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
	if ( GetArg( "--repl", argc, argv, &next ) )
	{
		QScript::Repl();
	}
	else if ( GetArg( "--file", argc, argv, &next ) )
	{
		QScript::FunctionObject* function = NULL;

		try
		{
			function = QScript::Compile( ReadFile( next ) );
			QScript::Interpret( *function );
		}
		catch ( std::vector< CompilerException >& exceptions )
		{
			for ( auto ex : exceptions )
				std::cout << ex.describe() << std::endl;
		}
		catch ( const RuntimeException& exception )
		{
			if ( exception.id() != "rt_exit" )
				std::cout << "Exception (" << exception.id() << "): " << exception.describe() << std::endl;
		}
		catch ( const Exception& exception )
		{
			std::cout << "Exception (" << exception.id() << "): " << exception.describe() << std::endl;
		}
		catch ( const std::exception& exception )
		{
			std::cout << "Exception: " << exception.what() << std::endl;
		}
		catch ( ... )
		{
			std::cout << "Unknown exception occurred." << std::endl;
		}

		QScript::FreeFunction( function );
	}
	else if ( GetArg( "--typer", argc, argv, &next ) )
	{
		for (;;)
		{
			try
			{
				std::string source;
				std::cout << "Typer >";
				std::getline( std::cin, source );

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
			catch ( std::vector< CompilerException >& exceptions )
			{
				for ( auto ex : exceptions )
					std::cout << ex.describe() << std::endl;
			}
			catch ( const Exception& exception )
			{
				std::cout << "Exception (" << exception.id() << "): " << exception.describe() << std::endl;
			}
			catch ( const std::exception& exception )
			{
				std::cout << "Exception: " << exception.what() << std::endl;
			}
			catch ( ... )
			{
				std::cout << "Unknown exception occurred." << std::endl;
			}
		}
	}

	return 0;
}
