namespace QScript
{
	enum OpCode : uint8_t
	{
		OP_LOAD_SHORT,
		OP_LOAD_LONG,
		OP_NOT,
		OP_NEG,
		OP_ADD,
		OP_SUB,
		OP_MUL,
		OP_DIV,
		OP_EQ,
		OP_NEQ,
		OP_GT,
		OP_LT,
		OP_LTE,
		OP_GTE,
		OP_PRINT,
		OP_POP,
		OP_RETN,
	};
}
