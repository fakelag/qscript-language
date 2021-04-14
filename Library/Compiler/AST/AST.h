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
		NT_TYPE,			// Type node, conveys a Type_t
	};

	enum NodeId
	{
		NODE_INVALID = -1,
		NODE_ACCESS_ARRAY,
		NODE_ACCESS_PROP,
		NODE_ADD,
		NODE_AND,
		NODE_ARGUMENTS,
		NODE_ARRAY,
		NODE_ASSIGN,
		NODE_ASSIGNADD,
		NODE_ASSIGNDIV,
		NODE_ASSIGNMOD,
		NODE_ASSIGNMUL,
		NODE_ASSIGNSUB,
		NODE_CALL,
		NODE_TABLE,
		NODE_CONSTANT,
		NODE_CONSTVAR,
		NODE_DEC,
		NODE_DIV,
		NODE_DO,
		NODE_EQUALS,
		NODE_FIELD,
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
		NODE_METHOD,
		NODE_NEG,
		NODE_NOT,
		NODE_NOTEQUALS,
		NODE_OR,
		NODE_POW,
		NODE_PROPERTYLIST,
		NODE_RETURN,
		NODE_SCOPE,
		NODE_SUB,
		NODE_TYPE,
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

	struct Argument_t
	{
		Argument_t( const std::string& name, Type_t* type, int lineNr, int colNr )
		{
			m_Name = name;
			m_Type = type;
			m_LineNr = lineNr;
			m_ColNr = colNr;

		}

		std::string 	m_Name;
		Type_t*			m_Type;
		int				m_LineNr;
		int				m_ColNr;
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
		void SetId( NodeId id )	{ m_NodeId = id; }

		virtual ~BaseNode() {}
		virtual void Release() {};
		virtual void Compile( Assembler& assembler, uint32_t options = CO_NONE ) = 0;
		virtual std::string ToJson( const std::string& ind = "" ) const = 0;

		virtual const Type_t* ExprType( Assembler& assembler ) = 0;

	protected:
		NodeId				m_NodeId;
		NodeType			m_NodeType;
		int					m_LineNr;
		int					m_ColNr;
		std::string 		m_Token;

		// Node expression type -- Updated and returned by ExprType()
		Type_t*				m_ExprType;
		Type_t*				m_ExprReturnType;
	};

	class TermNode : public BaseNode
	{
	public:
		TermNode( int lineNr, int colNr, const std::string token, NodeId id );

		void Release() override;
		void Compile( Assembler& assembler, uint32_t options = CO_NONE ) override;
		std::string ToJson( const std::string& ind = "" ) const override;
		const Type_t* ExprType( Assembler& assembler ) override;
	};

	class ValueNode : public BaseNode
	{
	public:
		ValueNode( int lineNr, int colNr, const std::string token, NodeId id, const QScript::Value& value );

		void Release() override;
		void Compile( Assembler& assembler, uint32_t options = CO_NONE ) override;
		const Type_t* ExprType( Assembler& assembler ) override;
		std::string ToJson( const std::string& ind = "" ) const override;

		QScript::Value& GetValue() { return m_Value; }
		const QScript::Value& GetValue() const { return m_Value; }
	private:
		QScript::Value		m_Value;
	};

	class ComplexNode : public BaseNode
	{
	public:
		ComplexNode( int lineNr, int colNr, const std::string token, NodeId id, BaseNode* left, BaseNode* right );

		void Release() override;
		void Compile( Assembler& assembler, uint32_t options = CO_NONE ) override;
		const Type_t* ExprType( Assembler& assembler ) override;
		std::string ToJson( const std::string& ind = "" ) const override;

		BaseNode* GetLeft();
		BaseNode* GetRight();

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
		const Type_t* ExprType( Assembler& assembler ) override;
		std::string ToJson( const std::string& ind = "" ) const override;

		BaseNode* GetNode();

	private:
		BaseNode*			m_Node;
	};

	class ListNode : public BaseNode
	{
	public:
		ListNode( int lineNr, int colNr, const std::string token, NodeId id, const std::vector< BaseNode* >& nodeList );

		void Release() override;
		void Compile( Assembler& assembler, uint32_t options = CO_NONE ) override;
		const Type_t* ExprType( Assembler& assembler ) override;
		const std::vector< BaseNode* >& GetList() const;
		std::string ToJson( const std::string& ind = "" ) const override;

	private:
		std::vector< BaseNode* >		m_NodeList;
	};

	class TypeNode : public BaseNode
	{
	public:
		TypeNode( int lineNr, int colNr, const std::string token, Type_t* type );

		void Release() override;
		void Compile( Assembler& assembler, uint32_t options = CO_NONE ) override;
		const Type_t* ExprType( Assembler& assembler ) override;
		const Type_t* GetType() const;
		std::string ToJson( const std::string& ind = "" ) const override;

	private:
		Type_t* m_Type;
	};

	std::vector< Argument_t > ParseArgsList( ListNode* argNode, Assembler& assembler );
}
