#pragma once
#define QS_OPCODES( processor ) \
processor(OP_ADD), \
processor(OP_CALL), \
processor(OP_CALL_0), \
processor(OP_CALL_1), \
processor(OP_CALL_2), \
processor(OP_CALL_3), \
processor(OP_CALL_4), \
processor(OP_CALL_5), \
processor(OP_CALL_6), \
processor(OP_CALL_7), \
processor(OP_DIV), \
processor(OP_EQUALS), \
processor(OP_SET_GLOBAL_LONG), \
processor(OP_SET_GLOBAL_SHORT), \
processor(OP_SET_LOCAL_LONG), \
processor(OP_SET_LOCAL_SHORT), \
processor(OP_GREATERTHAN), \
processor(OP_GREATERTHAN_OR_EQUAL), \
processor(OP_JUMP_BACK_SHORT), \
processor(OP_JUMP_BACK_LONG), \
processor(OP_JUMP_SHORT), \
processor(OP_JUMP_LONG), \
processor(OP_JUMP_IF_ZERO_SHORT), \
processor(OP_JUMP_IF_ZERO_LONG), \
processor(OP_LOAD_CONSTANT_LONG), \
processor(OP_LOAD_CONSTANT_SHORT), \
processor(OP_LOAD_GLOBAL_LONG), \
processor(OP_LOAD_GLOBAL_SHORT), \
processor(OP_LOAD_LOCAL_LONG), \
processor(OP_LOAD_LOCAL_SHORT), \
processor(OP_LESSTHAN), \
processor(OP_LESSTHAN_OR_EQUAL), \
processor(OP_MUL), \
processor(OP_NEGATE), \
processor(OP_NOT_EQUALS), \
processor(OP_NOT), \
processor(OP_LOAD_NULL), \
processor(OP_POP), \
processor(OP_PRINT), \
processor(OP_RETURN), \
processor(OP_SUB),

#define QS_OPCODE_PLAIN( opcode ) opcode

namespace QScript
{
	enum OpCode : uint8_t
	{
		QS_OPCODES( QS_OPCODE_PLAIN )
	};
}

#undef QS_OPCODE_PLAIN
