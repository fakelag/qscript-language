#include "QLibPCH.h"
#include "Compiler.h"
#include "Instructions.h"
#include "Exception.h"

namespace Compiler
{
	std::vector< BaseNode* > GenerateIR( const std::vector< Token_t >& tokens )
	{
		auto constant = new ValueNode( 0, 0, "1.2", NODE_CONSTANT, 1.2 );
		auto retn = new TermNode( 0, 0, "return", NODE_RETURN );
		auto constant2 = new ValueNode( 0, 0, "1.55", NODE_CONSTANT, 1.55 );

		return std::vector< BaseNode* >{ constant, constant2, retn };
	}

	BaseNode::BaseNode( int lineNr, int colNr, const std::string token, NodeType type, NodeId id )
	{
		m_LineNr		= lineNr;
		m_ColNr			= colNr;
		m_Token			= token;
		m_NodeType		= type;
		m_NodeId		= id;
	}

	TermNode::TermNode( int lineNr, int colNr, const std::string token, NodeId id )
		: BaseNode( lineNr, colNr, token, NT_TERM, id )
	{
	}

	void TermNode::Compile( QScript::Chunk_t* chunk )
	{
		// Call CompileXXX functions from Compiler.cpp for code generation,
		// but append debugging info to the chunk here
		CompileTermNode( m_NodeId, chunk );
	}

	ValueNode::ValueNode( int lineNr, int colNr, const std::string token, NodeId id, const QScript::Value& value )
		: BaseNode( lineNr, colNr, token, NT_VALUE, id )
	{
		m_Value = value;
	}

	void ValueNode::Compile( QScript::Chunk_t* chunk )
	{
		CompileValueNode( m_Value, chunk );
	}
}
