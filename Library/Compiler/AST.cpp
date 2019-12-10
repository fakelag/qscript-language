#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Compiler.h"
#include "Instructions.h"

namespace Compiler
{
	void AddDebugSymbol( QScript::Chunk_t* chunk, int start, int lineNr, int colNr, const std::string token )
	{
		if ( lineNr == -1 )
			return;

		chunk->m_Debug.push_back( QScript::Chunk_t::Debug_t{
			start,
			( int ) chunk->m_Code.size(),
			lineNr,
			colNr,
			token,
		} );
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
		int start = chunk->m_Code.size();

		switch( m_NodeId )
		{
		case NODE_RETURN:
			EmitByte( QScript::OpCode::OP_RETN, chunk );
			break;
		default:
			throw Exception( "cp_invalid_term_node", "Unknown terminating node: " + std::to_string( m_NodeId ) );
		}

		AddDebugSymbol( chunk, start, m_LineNr, m_ColNr, m_Token );
	}

	ValueNode::ValueNode( int lineNr, int colNr, const std::string token, NodeId id, const QScript::Value& value )
		: BaseNode( lineNr, colNr, token, NT_VALUE, id )
	{
		m_Value.From( value );
	}

	void ValueNode::Compile( QScript::Chunk_t* chunk )
	{
		int start = chunk->m_Code.size();

		int constantId = AddConstant( m_Value, chunk );

		EmitByte( QScript::OpCode::OP_LOAD, chunk );
		EmitByte( constantId, chunk );

		AddDebugSymbol( chunk, start, m_LineNr, m_ColNr, m_Token );
	}

	ComplexNode::ComplexNode( int lineNr, int colNr, const std::string token, NodeId id, BaseNode* left, BaseNode* right )
		: BaseNode( lineNr, colNr, token, NT_COMPLEX, id )
	{
		m_Left = left;
		m_Right = right;
	}

	ComplexNode::~ComplexNode()
	{
		delete m_Left;
		delete m_Right;
	}

	void ComplexNode::Compile( QScript::Chunk_t* chunk )
	{
		m_Left->Compile( chunk );
		m_Right->Compile( chunk );

		int start = chunk->m_Code.size();

		std::map< Compiler::NodeId, QScript::OpCode > singleByte = {
			{ NODE_ADD, 			QScript::OpCode::OP_ADD },
			{ NODE_SUB, 			QScript::OpCode::OP_SUB },
			{ NODE_MUL, 			QScript::OpCode::OP_MUL },
			{ NODE_DIV, 			QScript::OpCode::OP_DIV },
			{ NODE_EQUALS,			QScript::OpCode::OP_EQ },
			{ NODE_NOTEQUALS,		QScript::OpCode::OP_NEQ },
			{ NODE_GREATERTHAN,		QScript::OpCode::OP_GT },
			{ NODE_GREATEREQUAL,	QScript::OpCode::OP_GTE },
			{ NODE_LESSTHAN,		QScript::OpCode::OP_LT },
			{ NODE_LESSEQUAL,		QScript::OpCode::OP_LTE },
		};

		auto opCode = singleByte.find( m_NodeId );
		if (opCode != singleByte.end())
			EmitByte( opCode->second, chunk );
		else
			throw Exception( "cp_invalid_complex_node", "Unknown complex node: " + std::to_string( m_NodeId ) );

		AddDebugSymbol( chunk, start, m_LineNr, m_ColNr, m_Token );
	}

	SimpleNode::SimpleNode( int lineNr, int colNr, const std::string token, NodeId id, BaseNode* node )
		: BaseNode( lineNr, colNr, token, NT_SIMPLE, id )
	{
		m_Node = node;
	}

	SimpleNode::~SimpleNode()
	{
		delete m_Node;
	}

	void SimpleNode::Compile( QScript::Chunk_t* chunk )
	{
		m_Node->Compile( chunk );

		int start = chunk->m_Code.size();

		switch ( m_NodeId )
		{
		case NODE_NOT: EmitByte( QScript::OpCode::OP_NOT, chunk ); break;
		case NODE_NEG: EmitByte( QScript::OpCode::OP_NEG, chunk ); break;
		default:
			throw Exception( "cp_invalid_simple_node", "Unknown simple node: " + std::to_string( m_NodeId ) );
		}

		AddDebugSymbol( chunk, start, m_LineNr, m_ColNr, m_Token );
	}
}
