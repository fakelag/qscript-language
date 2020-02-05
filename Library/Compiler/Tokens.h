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
		BP_VAR						= 100,
		BP_OPENPAREN				= 110,
		BP_DOT						= 110,
		BP_TYPE						= 120,
	};

	enum Token
	{
		TOK_2EQUALS = 0,
		TOK_2MINUS,
		TOK_2PLUS,
		TOK_2STAR,
		TOK_AND,
		TOK_ARROW,
		TOK_BANG,
		TOK_BRACE_LEFT,
		TOK_BRACE_RIGHT,
		TOK_CLASS,
		TOK_COMMA,
		TOK_CONST,
		TOK_DBL,
		TOK_DO,
		TOK_DOT,
		TOK_ELSE,
		TOK_EQUALS,
		TOK_EQUALSADD,
		TOK_EQUALSSUB,
		TOK_EQUALSMUL,
		TOK_EQUALSDIV,
		TOK_EQUALSMOD,
		TOK_FALSE,
		TOK_FOR,
		TOK_FUNC,
		TOK_GREATEREQUAL,
		TOK_GREATERTHAN,
		TOK_IF,
		TOK_IMPORT,
		TOK_INT,
		TOK_LESSEQUAL,
		TOK_LESSTHAN,
		TOK_PAREN_LEFT,
		TOK_MINUS,
		TOK_NAME,
		TOK_NOTEQUALS,
		TOK_NULL,
		TOK_OR,
		TOK_PLUS,
		TOK_RETURN,
		TOK_PAREN_RIGHT,
		TOK_PERCENT,
		TOK_SLASH,
		TOK_STAR,
		TOK_STR,
		TOK_TRUE,
		TOK_VAR,
		TOK_SCOLON,
		TOK_WHILE,
	};

	struct Token_t
	{
		Token				m_Id;
		int 				m_LBP;
		int					m_LineNr;
		int					m_ColNr;
		std::string 		m_String;
	};
}
