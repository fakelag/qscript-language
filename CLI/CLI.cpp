#include "../Includes/QScript.h"

int main()
{
	QScript::Chunk_t chunk = QScript::Compile( "-8.2 - (6.0 + 2) + 4.5 * 4.2 - 5.2 + (-3 / 6) * 2 + ((4 + 6.0) - 2.0);" );
	QScript::Interpret( chunk );
	return 0;
}
