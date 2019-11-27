#include "../Includes/QScript.h"

int main()
{
	QScript::Chunk_t chunk = QScript::Compile( "1.5 + 5.5 + 1 + 90.5;" );
	QScript::Interpret( chunk );
	return 0;
}
