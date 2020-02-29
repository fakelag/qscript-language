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
		NODE_INLINE_IF,
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

	enum CompileTypeInfo : uint32_t
	{
		// Primitives
		TYPE_UNKNOWN			= ( 0 << 0 ),
		TYPE_NULL				= ( 1 << 0 ),
		TYPE_NUMBER				= ( 1 << 1 ),
		TYPE_BOOL				= ( 1 << 2 ),

		// Objects
		TYPE_CLASS				= ( 1 << 3 ),
		TYPE_CLOSURE			= ( 1 << 4 ),
		TYPE_FUNCTION			= ( 1 << 5 ),
		TYPE_INSTANCE			= ( 1 << 6 ),
		TYPE_NATIVE				= ( 1 << 7 ),
		TYPE_STRING				= ( 1 << 8 ),
		TYPE_UPVALUE			= ( 1 << 9 ),

		// No type (statements)
		TYPE_NONE				= ( 1 << 10 ),

		// Hint compiler to deduce type
		TYPE_AUTO				= ( 1 << 11 ),
	};

	struct Argument_t
	{
		std::string 	m_Name;
		uint32_t		m_Type;
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
		virtual std::string ToJson( const std::string& ind = "" ) const = 0;

		virtual uint32_t ExprType( Assembler& assembler ) const { return Compiler::TYPE_NONE; }

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
		std::string ToJson( const std::string& ind = "" ) const override;
	};

	class ValueNode : public BaseNode
	{
	public:
		ValueNode( int lineNr, int colNr, const std::string token, NodeId id, const QScript::Value& value );
		void Compile( Assembler& assembler, uint32_t options = CO_NONE ) override;
		uint32_t ExprType( Assembler& assembler ) const override;
		std::string ToJson( const std::string& ind = "" ) const override;

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
		uint32_t ExprType( Assembler& assembler ) const override;
		std::string ToJson( const std::string& ind = "" ) const override;

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
		uint32_t ExprType( Assembler& assembler ) const override;
		std::string ToJson( const std::string& ind = "" ) const override;

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
		uint32_t ExprType( Assembler& assembler ) const override;
		const std::vector< BaseNode* >& GetList() const;
		std::string ToJson( const std::string& ind = "" ) const override;

	private:
		std::vector< BaseNode* >		m_NodeList;
	};

	uint32_t ResolveReturnType( const ListNode* funcNode, Assembler& assembler );
	std::vector< Argument_t > ParseArgsList( ListNode* argNode );
}
