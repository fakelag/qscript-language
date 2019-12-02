#include <iostream>
#include "../Includes/QScript.h"
#include "../Includes/Exception.h"

int main()
{
	try
	{
		QScript::Chunk_t* chunk = QScript::Compile( "!null;!false;!!true;" );
		QScript::Interpret( *chunk );
	}
	catch ( const RuntimeException& exception )
	{
		std::cout << "Exception (" << exception.id() << "): " << exception.what() << std::endl;
	}
	catch ( const Exception& exception )
	{
		std::cout << "Exception (" << exception.id() << "): " << exception.what() << std::endl;
	}
	catch ( const std::exception& exception )
	{
		std::cout << "Exception: " << exception.what() << std::endl;
	}
	catch ( ... )
	{
		std::cout << "Unknown exception occurred." << std::endl;
	}

	return 0;
}
