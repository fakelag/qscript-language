#include "QLibPCH.h"
#include "../Library/Common/Chunk.h"
#include "../Library/Compiler/Compiler.h"
#include "../Library/Compiler/AST/AST.h"

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

	for ( ;;)
	{
		try
		{
			std::string source;
			std::string newLine;

			while ( std::getline( std::cin, newLine ) )
				source += newLine + "\n";

			auto mainFunction = QScript::Compile( source );
			std::cout << "OK" << std::endl;

			QScript::FreeFunction( mainFunction );
		}
		catch ( std::vector< CompilerException >& exceptions )
		{
			for ( auto ex : exceptions )
				std::cout << "[" + ex.id() + "]:" + ex.describe() << std::endl;
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
