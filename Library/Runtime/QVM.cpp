#include "../Includes/QLibPCH.h"
#include "../Includes/Instructions.h"

#include "../Compiler/Compiler.h"

void QScript::Interpret( const Chunk_t& chunk )
{
	Compiler::DisassembleChunk( chunk, "main" );
}
