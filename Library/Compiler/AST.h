#pragma once

namespace Compiler
{
	enum NodeType
	{
		NT_INVALID = -1,
		NT_TERM,			// Terminating node, links to no further nodes
		NT_VALUE,			// Value node. Contains a value, links to no further nodes
		// NT_SIMPLE,			// Simple node, links to a single subtree (of 1...n items)
	};

	enum NodeId
	{
		NODE_INVALID = -1,
		NODE_CONSTANT,
		NODE_RETURN,
	};

	class BaseNode
	{
	public:
		BaseNode( int lineNr, int colNr, const std::string token, NodeType type, NodeId id );

		NodeType Type()		const { return m_NodeType; }
		NodeId Id()			const { return m_NodeId; }

		virtual ~BaseNode() {}
		virtual void Compile( QScript::Chunk_t* chunk ) = 0;

	protected:
		NodeId				m_NodeId;
		NodeType			m_NodeType;
		int					m_LineNr;
		int					m_ColNr;
		std::string 		m_Token;
	};

	class TermNode : public BaseNode
	{
	public:
		TermNode( int lineNr, int colNr, const std::string token, NodeId id );
		void Compile( QScript::Chunk_t* chunk );
	};

	class ValueNode : public BaseNode
	{
	public:
		ValueNode( int lineNr, int colNr, const std::string token, NodeId id, const QScript::Value& value );
		void Compile( QScript::Chunk_t* chunk );

	private:
		QScript::Value		m_Value;
	};
}
