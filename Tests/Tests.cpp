#include "QLibPCH.h"
#include "Tests.h"

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

int main( int argc, const char** argv )
{
#if defined(_WIN32) || defined(_WIN64)
	// Enable ANSI colors for windows console
	HANDLE hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
	DWORD dwMode = 0;
	GetConsoleMode( hStdOut, &dwMode );
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode( hStdOut, dwMode );
#endif

	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	{
		bool allPassed = true;
		std::vector< bool > testResults;

		testResults.push_back( Tests::TestLexer() );
		testResults.push_back( Tests::TestCompiler() );
		testResults.push_back( Tests::TestInterpreter() );

		for ( auto result : testResults )
			if ( !result ) allPassed = false;

		if ( allPassed )
			std::cout << "\n\n\033[32mAll tests passed\033[39m" << std::endl;
		else
			std::cout << "\n\n\033[31mSome tests failed\033[39m" << std::endl;

#ifdef _WIN32
		std::getchar();
#endif
	}

	return 0;
}