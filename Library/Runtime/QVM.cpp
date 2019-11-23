#include "QLibPCH.h"
#include "Instructions.h"
#include "../Compiler/Compiler.h"

void QScript::Interpret( const Chunk_t& chunk )
{
	Compiler::DisassembleChunk( chunk, "main" );
}
