#pragma once

namespace Compiler
{
	class Assembler;

	enum NodeType
	{
		NT_INVALID = -1,
		NT_TERM,			// Terminating node, links to no further nodes
		NT_VALUE,			// Value node. Contains a value, links to no further nodes
		NT_SIMPLE,			// Simple node, links to a single subtree (of 1...n items)
		NT_COMPLEX,			// Complex node, links to two (left & right) nodes
		NT_LIST,			// List node, 0...n child nodes
	};

	enum NodeId
	{
		NODE_INVALID = -1,
		NODE_ACCESS_PROP,
		NODE_ADD,
		NODE_AND,
		NODE_ARGUMENTS,
		NODE_ASSIGN,
		NODE_ASSIGNADD,
		NODE_ASSIGNDIV,
		NODE_ASSIGNMOD,
		NODE_ASSIGNMUL,
		NODE_ASSIGNSUB,
		NODE_CALL,
		NODE_CLASS,
		NODE_CONSTANT,
		NODE_CONSTVAR,
		NODE_DEC,
		NODE_DIV,
		NODE_DO,
		NODE_EQUALS,
		NODE_FOR,
		NODE_FUNC,
		NODE_GREATEREQUAL,
		NODE_GREATERTHAN,
		NODE_IF,
		NODE_IMPORT,
		NODE_INC,
		NODE_LESSEQUAL,
		NODE_LESSTHAN,
		NODE_MOD,
		NODE_MUL,
		NODE_NAME,
		NODE_NEG,
		NODE_NOT,
		NODE_NOTEQUALS,
		NODE_OR,
		NODE_POW,
		NODE_RETURN,
		NODE_SCOPE,
		NODE_SUB,
		NODE_VAR,
		NODE_WHILE,
	};

	enum CompileOptions : uint32_t
	{
		CO_NONE					= ( 0 << 0 ),
		CO_ASSIGN				= ( 1 << 0 ),
		CO_REASSIGN				= ( 1 << 1 ),

		// Expressions produce values, inform subsequent nodes that they should compile
		// an expression.
		CO_EXPRESSION			= ( 1 << 2 ),
	};

	class BaseNode
	{
	public:
		BaseNode( int lineNr, int colNr, const std::string token, NodeType type, NodeId id );

		NodeType Type()			const { return m_NodeType; }
		NodeId Id()				const { return m_NodeId; }
		int LineNr()			const { return m_LineNr; }
		int ColNr()				const { return m_ColNr; }
		std::string Token()		const { return m_Token; }

		virtual ~BaseNode() {}
		virtual void Release() {};
		virtual void Compile( Assembler& assembler, uint32_t options = CO_NONE ) = 0;

		virtual bool IsString() const { return false; }

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
		void Compile( Assembler& assembler, uint32_t options = CO_NONE ) override;
	};

	class ValueNode : public BaseNode
	{
	public:
		ValueNode( int lineNr, int colNr, const std::string token, NodeId id, const QScript::Value& value );
		void Compile( Assembler& assembler, uint32_t options = CO_NONE ) override;

		bool IsString() const override;

		QScript::Value& GetValue() { return m_Value; }
	private:
		QScript::Value		m_Value;
	};

	class ComplexNode : public BaseNode
	{
	public:
		ComplexNode( int lineNr, int colNr, const std::string token, NodeId id, BaseNode* left, BaseNode* right );

		void Release() override;
		void Compile( Assembler& assembler, uint32_t options = CO_NONE ) override;

		const BaseNode* GetLeft() const;
		const BaseNode* GetRight() const;

	private:
		BaseNode*			m_Left;
		BaseNode*			m_Right;
	};

	class SimpleNode : public BaseNode
	{
	public:
		SimpleNode( int lineNr, int colNr, const std::string token, NodeId id, BaseNode* node );

		void Release() override;
		void Compile( Assembler& assembler, uint32_t options = CO_NONE ) override;

		const BaseNode* GetNode() const;

	private:
		BaseNode*			m_Node;
	};

	class ListNode : public BaseNode
	{
	public:
		ListNode( int lineNr, int colNr, const std::string token, NodeId id, const std::vector< BaseNode* >& nodeList );

		void Release() override;
		void Compile( Assembler& assembler, uint32_t options = CO_NONE ) override;
		const std::vector< BaseNode* >& GetList() const;

	private:
		std::vector< BaseNode* >		m_NodeList;
	};
}
