#include "../Includes/QScript.h"

int main()
{
	QScript::Chunk_t chunk = QScript::Compile( "1 + 1" );
	QScript::Interpret( chunk );
	return 0;
}
