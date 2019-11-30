#pragma once

namespace Compiler
{
	enum BindingPower
	{
		BP_IMMEDIATE				= -1,
		BP_NONE						= 0,
		BP_COMMA					= 10,
		BP_ASSIGN					= 20,
		BP_LOGIC_NOT				= 30,
		BP_LOGIC					= 40,
		BP_EQUALITY					= 50,
		BP_ARITHMETIC_ADDSUB		= 60,
		BP_ARITHMETIC_MULDIV		= 70,
		BP_ARITHMETIC_POWMOD		= 80,
		BP_INCDEC					= 90,
		BP_OPENBRACKET				= 100,
		BP_DOT						= 110,
		BP_TYPE						= 120,
	};

	enum Token
	{
		TOK_SCOLON = 0,
		TOK_NAME,
		TOK_INT,
		TOK_DBL,
		TOK_STR,
		TOK_PLUS,
		TOK_MINUS,
		TOK_STAR,
		TOK_SLASH,
		TOK_LPAREN,
		TOK_RPAREN,
		TOK_TRUE,
		TOK_FALSE,
		TOK_NULL,
	};

	struct Token_t
	{
		Token				m_Token;
		int 				m_LBP;
		int					m_LineNr;
		int					m_ColNr;
		std::string 		m_String;
	};
}
