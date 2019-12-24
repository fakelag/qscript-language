namespace QScript
{
	enum OpCode : uint8_t
	{
		OP_ADD,			// Addition
		OP_DIV,			// Division
		OP_EQ,			// Equals
		OP_SG_LONG,		// Set Global Long
		OP_SG_SHORT,	// Set Global Short
		OP_SL_SHORT,	// Set Local Short
		OP_SL_LONG,		// Set Local Long
		OP_GT,			// Greater Than
		OP_GTE,			// Greater Than or Equal
		OP_LD_LONG,		// Load Long
		OP_LD_SHORT,	// Load Short
		OP_LG_LONG,		// Load Global Long
		OP_LG_SHORT,	// Load Global Short
		OP_LL_LONG,		// Load Local Long
		OP_LL_SHORT,	// Load Local Short
		OP_LT,			// Less Than
		OP_LTE,			// Less Than or Equal
		OP_MUL,			// Multiply
		OP_NEG,			// Negate
		OP_NEQ,			// Not Equals
		OP_NOT,
		OP_PNULL,		// Push Null
		OP_POP,
		OP_PRINT,
		OP_RETN,		// Return (with exit code)
		OP_SUB,			// Subtract
	};
}
