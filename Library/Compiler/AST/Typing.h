#pragma once

// Include Type_t
#include "../Types.h"

namespace Compiler
{
	class Assembler;
	class ListNode;

	void ResolveReturnType( ListNode* funcNode, Assembler& assembler, Type_t* out );
}
