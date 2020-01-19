#pragma once
#define QS_OPCODES( fmt ) \
fmt(OP_ADD), \
fmt(OP_CALL), \
fmt(OP_CALL_0), \
fmt(OP_CALL_1), \
fmt(OP_CALL_2), \
fmt(OP_CALL_3), \
fmt(OP_CALL_4), \
fmt(OP_CALL_5), \
fmt(OP_CALL_6), \
fmt(OP_CALL_7), \
fmt(OP_CLOSE_UPVALUE), \
fmt(OP_CLOSURE_SHORT), \
fmt(OP_CLOSURE_LONG), \
fmt(OP_DIV), \
fmt(OP_EQUALS), \
fmt(OP_SET_UPVALUE_SHORT), \
fmt(OP_SET_UPVALUE_LONG), \
fmt(OP_GREATERTHAN), \
fmt(OP_GREATERTHAN_OR_EQUAL), \
fmt(OP_JUMP_BACK_SHORT), \
fmt(OP_JUMP_BACK_LONG), \
fmt(OP_JUMP_SHORT), \
fmt(OP_JUMP_LONG), \
fmt(OP_JUMP_IF_ZERO_SHORT), \
fmt(OP_JUMP_IF_ZERO_LONG), \
fmt(OP_LOAD_CONSTANT_LONG), \
fmt(OP_LOAD_CONSTANT_SHORT), \
fmt(OP_LOAD_GLOBAL_LONG), \
fmt(OP_LOAD_GLOBAL_SHORT), \
fmt(OP_LOAD_LOCAL_LONG), \
fmt(OP_LOAD_LOCAL_SHORT), \
fmt(OP_LOAD_LOCAL_0), \
fmt(OP_LOAD_LOCAL_1), \
fmt(OP_LOAD_LOCAL_2), \
fmt(OP_LOAD_LOCAL_3), \
fmt(OP_LOAD_LOCAL_4), \
fmt(OP_LOAD_LOCAL_5), \
fmt(OP_LOAD_LOCAL_6), \
fmt(OP_LOAD_LOCAL_7), \
fmt(OP_LOAD_LOCAL_8), \
fmt(OP_LOAD_LOCAL_9), \
fmt(OP_LOAD_LOCAL_10), \
fmt(OP_LOAD_LOCAL_11), \
fmt(OP_LOAD_UPVALUE_SHORT), \
fmt(OP_LOAD_UPVALUE_LONG), \
fmt(OP_LESSTHAN), \
fmt(OP_LESSTHAN_OR_EQUAL), \
fmt(OP_MUL), \
fmt(OP_NEGATE), \
fmt(OP_NOP), \
fmt(OP_NOT_EQUALS), \
fmt(OP_NOT), \
fmt(OP_LOAD_NULL), \
fmt(OP_POP), \
fmt(OP_PRINT), \
fmt(OP_RETURN), \
fmt(OP_SET_GLOBAL_LONG), \
fmt(OP_SET_GLOBAL_SHORT), \
fmt(OP_SET_LOCAL_LONG), \
fmt(OP_SET_LOCAL_SHORT), \
fmt(OP_SET_LOCAL_0), \
fmt(OP_SET_LOCAL_1), \
fmt(OP_SET_LOCAL_2), \
fmt(OP_SET_LOCAL_3), \
fmt(OP_SET_LOCAL_4), \
fmt(OP_SET_LOCAL_5), \
fmt(OP_SET_LOCAL_6), \
fmt(OP_SET_LOCAL_7), \
fmt(OP_SET_LOCAL_8), \
fmt(OP_SET_LOCAL_9), \
fmt(OP_SET_LOCAL_10), \
fmt(OP_SET_LOCAL_11), \
fmt(OP_SUB),

#define QS_OPCODE_PLAIN( opcode ) opcode

namespace QScript
{
	enum OpCode : uint8_t
	{
		QS_OPCODES( QS_OPCODE_PLAIN )
		OP_CALL_MAX = OP_CALL_7 - OP_CALL,
		OP_LOAD_LOCAL_MAX = OP_LOAD_LOCAL_11 - OP_LOAD_LOCAL_0 + 1,
		OP_SET_LOCAL_MAX = OP_SET_LOCAL_11 - OP_SET_LOCAL_0 + 1,
	};
}

#undef QS_OPCODE_PLAIN
