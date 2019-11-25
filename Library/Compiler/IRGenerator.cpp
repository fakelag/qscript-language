#include "QLibPCH.h"
#include "Compiler.h"
#include "Instructions.h"
#include "Exception.h"
#include "IR.h"

namespace Compiler
{
	std::vector< BaseNode* > GenerateIR( const std::vector< Token_t >& tokens )
	{
		ParserState parserState;

		auto nextExpression = [ &parserState ]( int rbp = 0 ) -> BaseNode*
		{
			auto builder = parserState.NextBuilder();
			auto left = builder->m_Nud( *builder );

			while ( rbp < parserState.CurrentBuilder()->m_Token.m_LBP )
			{
				builder = parserState.NextBuilder();
				left = builder->m_Led( *builder, left );
			}

			return left;
		};

		auto createBuilder = []( const Token_t& token ) -> IrBuilder_t*
		{
			IrBuilder_t* builder = new IrBuilder_t( token );

			switch ( token.m_Token )
			{
			case Compiler::TOK_DBL:
			case Compiler::TOK_INT:
			{
				builder->m_Nud = []( const IrBuilder_t& irBuilder )
				{
					auto node = new ValueNode( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_CONSTANT, std::stod( irBuilder.m_Token.m_String ) );

					return node;
				};
				break;
			}
			case Compiler::TOK_SCOLON:
				break;
			default:
				throw Exception( "ir_unknown_token", std::string( "Unknown token id: " )
					+ std::to_string( token.m_Token )
					+ " \"" +  token.m_String  + "\"" );
			}

			return builder;
		};

		std::transform( tokens.begin(), tokens.end(), std::back_inserter( parserState.Builders() ), createBuilder );

		while ( !parserState.IsFinished() )
		{
			parserState.AddNode( nextExpression() );
			parserState.NextBuilder();
		}

		parserState.AddNode( new TermNode( 0, 0, "", NODE_RETURN ) );
		return parserState.Product();
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
