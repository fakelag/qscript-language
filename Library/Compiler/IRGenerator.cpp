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

		// TDOP expression parsing
		auto nextExpression = [ &parserState ]( int rbp = 0 ) -> BaseNode*
		{
			// Get the current builder, increment counter to the next one
			auto builder = parserState.NextBuilder();

			// parse null-denoted node
			auto left = builder->m_Nud( *builder );

			if ( rbp == -1 )
				return left;

			// while the next builder has a larger binding power
			// deliver it the left hand node instead
			while ( rbp < parserState.CurrentBuilder()->m_Token.m_LBP )
			{
				builder = parserState.NextBuilder();
				left = builder->m_Led( *builder, left );
			}

			return left;
		};

		auto createBuilder = [ &parserState, &nextExpression ]( const Token_t& token ) -> IrBuilder_t*
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
			case Compiler::TOK_MINUS:
			{
				builder->m_Nud = [ &nextExpression ]( const IrBuilder_t& irBuilder )
				{
					auto node = new SimpleNode( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_NEG, nextExpression( -1 ) );

					return node;
				};
				/* Fall Through */
			}
			case Compiler::TOK_PLUS:
			case Compiler::TOK_SLASH:
			case Compiler::TOK_STAR:
			{
				builder->m_Led = [ &nextExpression ]( const IrBuilder_t& irBuilder, BaseNode* left )
				{
					NodeId nodeId = NODE_ADD;
					switch ( irBuilder.m_Token.m_Token )
					{
						case Compiler::TOK_MINUS: nodeId = NODE_SUB; break;
						case Compiler::TOK_STAR: nodeId = NODE_MUL; break;
						case Compiler::TOK_SLASH: nodeId = NODE_DIV; break;
						default: break;
					}

					auto node = new ComplexNode( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, nodeId, left, nextExpression( irBuilder.m_Token.m_LBP ) );

					return node;
				};
				break;
			}
			case Compiler::TOK_RPAREN:
			case Compiler::TOK_SCOLON:
				break;
			case Compiler::TOK_LPAREN:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder )
				{
					// TODO: check for instant closing rparen

					auto expression = nextExpression();

					if ( parserState.NextBuilder()->m_Token.m_Token != Compiler::TOK_RPAREN )
					{
						auto curBuilder = parserState.CurrentBuilder();
						throw Exception( "ir_missing_rparen",
							std::string( "Expected an end of expression, got: \"" + curBuilder->m_Token.m_String + "\"" ) );
					}

					return expression;
				};

				break;
			}
			default:
				throw Exception( "ir_unknown_token", std::string( "Unknown token id: " )
					+ std::to_string( token.m_Token )
					+ " \"" +  token.m_String  + "\"" );
			}

			return builder;
		};

		// Map all tokens to IR builders
		std::transform( tokens.begin(), tokens.end(), std::back_inserter( parserState.Builders() ), createBuilder );

		// Parse from top-level down
		while ( !parserState.IsFinished() )
		{
			parserState.AddNode( nextExpression() );
			parserState.NextBuilder();
		}

		// Add a last NODE_RETURN to exit the program
		parserState.AddNode( new TermNode( -1, -1, "", NODE_RETURN ) );

		// Builders are freed once parserState gets destructed
		return parserState.Product();
	}
}
