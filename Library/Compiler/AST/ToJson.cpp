#include "QLibPCH.h"
#include "../../Common/Value.h"
#include "AST.h"

namespace Compiler
{
	std::map< NodeId, std::string > nodeIdToName = {
		{ NODE_INVALID,				"INVALID" },
		{ NODE_ACCESS_ARRAY,		"ACCESS ARRAY" },
		{ NODE_ACCESS_PROP,			"ACCESS PROP" },
		{ NODE_ADD,					"ADD" },
		{ NODE_AND,					"AND" },
		{ NODE_ARGUMENTS,			"ARGUMENTS" },
		{ NODE_ASSIGN,				"ASSIGN" },
		{ NODE_ASSIGNADD,			"ASSIGN+" },
		{ NODE_ASSIGNDIV,			"ASSIGN/" },
		{ NODE_ASSIGNMOD,			"ASSIGN%" },
		{ NODE_ASSIGNMUL,			"ASSIGN*" },
		{ NODE_ASSIGNSUB,			"ASSIGN-" },
		{ NODE_CALL,				"CALL" },
		{ NODE_TABLE,				"TABLE" },
		{ NODE_CONSTANT,			"CONSTANT" },
		{ NODE_CONSTVAR,			"VAR (const)" },
		{ NODE_DEC,					"DECREMENT" },
		{ NODE_DIV,					"DIV" },
		{ NODE_DO,					"DO" },
		{ NODE_EQUALS,				"EQUALS" },
		{ NODE_FOR,					"FOR" },
		{ NODE_FUNC,				"FUNCTION" },
		{ NODE_GREATEREQUAL,		"GREATER OR EQUAL" },
		{ NODE_GREATERTHAN,			"GREATER THAN" },
		{ NODE_IF,					"IF" },
		{ NODE_IMPORT,				"IMPORT" },
		{ NODE_INC,					"INCREMENT" },
		{ NODE_INLINE_IF,			"INLINE IF" },
		{ NODE_LESSEQUAL,			"LESS OR EQUAL" },
		{ NODE_LESSTHAN,			"LESS THAN" },
		{ NODE_MOD,					"MOD" },
		{ NODE_MUL,					"MUL" },
		{ NODE_NAME,				"NAME" },
		{ NODE_NEG,					"NEG" },
		{ NODE_NOT,					"NOT" },
		{ NODE_NOTEQUALS,			"NOT EQUALS" },
		{ NODE_OR,					"OR" },
		{ NODE_POW,					"POW" },
		{ NODE_RETURN,				"RETURN" },
		{ NODE_SCOPE,				"SCOPE" },
		{ NODE_SUB,					"SUB" },
		{ NODE_VAR,					"VAR (mutable)" },
		{ NODE_WHILE,				"WHILE" },
	};

	std::string TermNode::ToJson( const std::string& ind ) const
	{
		std::string ind2 = ind + " ";
		return ind + "{\n"
			+ ind2 + "\"type\": \"TermNode\",\n"
			+ ind2 + "\"name\": \"" + nodeIdToName[ m_NodeId ] + "\",\n"
			+ ind + "}";
	}

	std::string ValueNode::ToJson( const std::string& ind ) const
	{
		std::string ind2 = ind + " ";
		return ind + "{\n"
			+ ind2 + "\"type\": \"ValueNode\",\n"
			+ ind2 + "\"name\": \"" + nodeIdToName[ m_NodeId ] + "\",\n"
			+ ind2 + "\"value\": \"" + m_Value.ToString() + "\"\n"
			+ ind + "}";
	}

	std::string ComplexNode::ToJson( const std::string& ind ) const
	{
		std::string ind2 = ind + " ";
		return ind + "{\n"
			+ ind2 + "\"type\": \"ComplexNode\",\n"
			+ ind2 + "\"name\": \"" + nodeIdToName[ m_NodeId ] + "\",\n"
			+ ind2 + "\"left\": \n" + ( m_Left ? m_Left->ToJson( ind2 ) : "NULL" ) + ",\n"
			+ ind2 + "\"right\": \n" + ( m_Right ? m_Right->ToJson( ind2 ) : "NULL" ) + "\n"
			+ ind + "}";
	}

	std::string SimpleNode::ToJson( const std::string& ind ) const
	{
		std::string ind2 = ind + " ";
		return ind + "{\n"
			+ ind2 + "\"type\": \"SimpleNode\",\n"
			+ ind2 + "\"name\": \"" + nodeIdToName[ m_NodeId ] + "\",\n"
			+ ind2 + "\"node\": \n" + m_Node->ToJson( ind2 ) + "\n"
			+ ind + "}";
	}

	std::string ListNode::ToJson( const std::string& ind ) const
	{
		std::string ind2 = ind + " ";

		std::string listJson = "";
		for ( size_t i = 0; i < m_NodeList.size(); ++i )
		{
			listJson += m_NodeList[ i ] ? m_NodeList[ i ]->ToJson( ind2 ) : "NULL";

			if ( i < m_NodeList.size() - 1 )
				listJson += ",";
		}

		return ind + "{\n"
			+ ind2 + "\"type\": \"ListNode\",\n"
			+ ind2 + "\"name\": \"" + nodeIdToName[ m_NodeId ] + "\",\n"
			+ ind2 + "\"list\": [\n" + listJson + "]\n"
			+ ind + "}";
	}
}
