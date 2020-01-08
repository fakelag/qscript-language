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
}
