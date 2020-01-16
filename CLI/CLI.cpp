#include "QLibPCH.h"

#include "../Includes/QScript.h"
#include "../Includes/Exception.h"

int main()
{
	std::string command;
	for ( ;; )
	{
		// std::cout << "REPL > ";
		// std::getline( std::cin, command );

		try
		{
			QScript::Function_t* function = QScript::Compile( "function a = () { var x =4; function b = () { x = 2; } b(); return x; } return a();" ); // command );
			QScript::Interpret( *function, NULL );

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
