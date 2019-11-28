#include "../Includes/QScript.h"

int main()
{
	QScript::Chunk_t chunk = QScript::Compile( "(6.2 + 2) + 4 * 4 - 5 + 3 / 6 * 2 + ((4 + 6.5) - 2.2);" );
	QScript::Interpret( chunk );
	return 0;
}
