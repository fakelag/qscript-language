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

int main()
{
	std::string command;
	for ( ;; )
	{
		//std::cout << "REPL > ";
		//std::getline( std::cin, command );

		try
		{
			QScript::FunctionObject* function = QScript::Compile( ReadFile( "program.qss" ) );
			QScript::Interpret( *function );

			QScript::FreeFunction( function );
		}
		catch ( std::vector< CompilerException >& exceptions )
		{
			for ( auto ex : exceptions )
				std::cout << ex.describe() << std::endl;
		}
		catch ( const RuntimeException& exception )
		{
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

		// Flush stdin
		int ch;
		while ( ( ch = std::cin.get() ) != '\n' && ch != EOF );
	}

	return 0;
}
