#include "QLibPCH.h"
#include "Compiler.h"

namespace Compiler
{
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
		int start = chunk->m_Code.size();

		CompileTermNode( m_NodeId, chunk );

		if ( m_LineNr != -1 )
		{
			chunk->m_Debug.push_back( QScript::Chunk_t::Debug_t{
				start,
				( int ) chunk->m_Code.size(),
				m_LineNr,
				m_ColNr,
				m_Token,
			} );
		}
	}

	ValueNode::ValueNode( int lineNr, int colNr, const std::string token, NodeId id, const QScript::Value& value )
		: BaseNode( lineNr, colNr, token, NT_VALUE, id )
	{
		m_Value = value;
	}

	void ValueNode::Compile( QScript::Chunk_t* chunk )
	{
		int start = chunk->m_Code.size();
		CompileValueNode( m_Value, chunk );

		if ( m_LineNr != -1 )
		{
			chunk->m_Debug.push_back( QScript::Chunk_t::Debug_t{
				start,
				( int ) chunk->m_Code.size(),
				m_LineNr,
				m_ColNr,
				m_Token,
			} );
		}
	}
}