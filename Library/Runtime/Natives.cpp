#include "QLibPCH.h"
#include "Natives.h"
#include "../Common/Chunk.h"

#include <time.h>

namespace Native
{
	QScript::Value clock( const QScript::Value* args, int numArgs )
	{
		return MAKE_NUMBER( (double) ::clock() / CLOCKS_PER_SEC );
	}

	QScript::Value exit( const QScript::Value* args, int numArgs )
	{
		throw RuntimeException( "rt_exit", "exit() called", 0, 0, "" );
	}
}
