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
}
