#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Compiler.h"
#include "Instructions.h"
#include "IR.h"

namespace Compiler
{
	std::vector< BaseNode* > GenerateIR( const std::vector< Token_t >& tokens )
	{
		ParserState parserState;

		// TDOP expression parsing
		auto nextExpression = [ &parserState ]( int rbp = 0 ) -> BaseNode*
		{
			if ( parserState.IsFinished() )
				throw CompilerException( "ir_parsing_past_eof", "Parsing past end of file", -1, -1, "" );

			// Get the current builder, increment counter to the next one
			auto builder = parserState.NextBuilder();

			if ( builder->m_Nud == NULL )
			{
				throw CompilerException( "ir_expect_lvalue_or_statement", "Expected a left-value or statement",
					builder->m_Token.m_LineNr, builder->m_Token.m_ColNr, builder->m_Token.m_String );
			}

			// parse null-denoted node
			auto left = builder->m_Nud( *builder );

			if ( rbp == -1 )
				return left;

			if ( parserState.IsFinished() )
				throw CompilerException( "ir_parsing_past_eof", "Parsing past end of file", -1, -1, "" );

			// while the next builder has a larger binding power
			// deliver it the left hand node instead
			while ( rbp < parserState.CurrentBuilder()->m_Token.m_LBP )
			{
				builder = parserState.NextBuilder();

				if ( builder->m_Led == NULL )
				{
					throw CompilerException( "ir_expect_rvalue", "Expected a right-value",
						builder->m_Token.m_LineNr, builder->m_Token.m_ColNr, builder->m_Token.m_String );
				}

				left = builder->m_Led( *builder, left );
			}

			return left;
		};

		auto createBuilder = [ &parserState, &nextExpression ]( const Token_t& token ) -> IrBuilder_t*
		{
			IrBuilder_t* builder = new IrBuilder_t( token );

			switch ( token.m_Token )
			{
			case Compiler::TOK_NULL:
			case Compiler::TOK_FALSE:
			case Compiler::TOK_TRUE:
			case Compiler::TOK_DBL:
			case Compiler::TOK_INT:
			case Compiler::TOK_STR:
			{
				builder->m_Nud = [ &parserState ]( const IrBuilder_t& irBuilder )
				{
					QScript::Value value;

					switch ( irBuilder.m_Token.m_Token )
					{
						case Compiler::TOK_STR:
							value.From( MAKE_STRING( irBuilder.m_Token.m_String ) );
							break;
						case Compiler::TOK_INT:
							value.From( MAKE_NUMBER( ( double ) std::stoi( irBuilder.m_Token.m_String ) ) );
							break;
						case Compiler::TOK_DBL:
							value.From( MAKE_NUMBER( std::stod( irBuilder.m_Token.m_String ) ) );
							break;
						case Compiler::TOK_NULL:
							value.From( MAKE_NULL );
							break;
						case Compiler::TOK_FALSE:
							value.From( MAKE_BOOL( false ) );
							break;
						case Compiler::TOK_TRUE:
							value.From( MAKE_BOOL( true ) );
							break;
						default:
						{
							throw CompilerException( "ir_invalid_token", "Invalid token: \"" + irBuilder.m_Token.m_String + "\"",
								irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr, irBuilder.m_Token.m_String );
						}
					};

					return parserState.AllocateNode< ValueNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_CONSTANT, value );
				};
				break;
			}
			case Compiler::TOK_NAME:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder ) -> BaseNode*
				{
					if ( irBuilder.m_Token.m_String == "print" )
					{
						return parserState.AllocateNode< SimpleNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
							irBuilder.m_Token.m_String, NODE_PRINT, nextExpression( irBuilder.m_Token.m_LBP ) );
					}
					else
					{
						return parserState.AllocateNode< ValueNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
							irBuilder.m_Token.m_String, NODE_NAME, MAKE_STRING( irBuilder.m_Token.m_String ) );
					}
				};
				break;
			}
			case Compiler::TOK_VAR:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder )
				{
					auto varName = nextExpression( irBuilder.m_Token.m_LBP );

					if ( !varName->IsString() )
					{
						auto builder = parserState.CurrentBuilder();
						throw CompilerException( "ir_invalid_token", "Invalid token: \"" + builder->m_Token.m_String + "\"",
							builder->m_Token.m_LineNr, builder->m_Token.m_ColNr, builder->m_Token.m_String );
					}

					return parserState.AllocateNode< SimpleNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_VAR, varName );
				};
				break;
			}
			case Compiler::TOK_BANG:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder )
				{
					return parserState.AllocateNode< SimpleNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_NOT, nextExpression( irBuilder.m_Token.m_LBP ) );
				};
				break;
			}
			case Compiler::TOK_MINUS:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder )
				{
					return parserState.AllocateNode< ValueNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_NEG, nextExpression( -1 ) );
				};
				/* Fall Through */
			}
			case Compiler::TOK_PLUS:
			case Compiler::TOK_SLASH:
			case Compiler::TOK_STAR:
			{
				builder->m_Led = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder, BaseNode* left )
				{
					std::map<Compiler::Token, Compiler::NodeId> map = {
						{ Compiler::TOK_MINUS, 	Compiler::NODE_SUB },
						{ Compiler::TOK_STAR, 	Compiler::NODE_MUL },
						{ Compiler::TOK_SLASH, 	Compiler::NODE_DIV },
						{ Compiler::TOK_PLUS, 	Compiler::NODE_ADD },
					};

					return parserState.AllocateNode< ComplexNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, map[ irBuilder.m_Token.m_Token ], left, nextExpression( irBuilder.m_Token.m_LBP ) );
				};
				break;
			}
			case Compiler::TOK_EQUALS:
			{
				builder->m_Led = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder, BaseNode* left )
				{
					return parserState.AllocateNode< ComplexNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_ASSIGN, left, nextExpression( irBuilder.m_Token.m_LBP ) );
				};
				break;
			}
			case Compiler::TOK_NOTEQUALS:
			case Compiler::TOK_2EQUALS:
			case Compiler::TOK_GREATERTHAN:
			case Compiler::TOK_GREATEREQUAL:
			case Compiler::TOK_LESSTHAN:
			case Compiler::TOK_LESSEQUAL:
			{
				builder->m_Led = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder, BaseNode* left )
				{
					std::map<Compiler::Token, Compiler::NodeId> map = {
						{ Compiler::TOK_2EQUALS, 		Compiler::NODE_EQUALS },
						{ Compiler::TOK_NOTEQUALS, 		Compiler::NODE_NOTEQUALS },
						{ Compiler::TOK_GREATERTHAN, 	Compiler::NODE_GREATERTHAN },
						{ Compiler::TOK_GREATEREQUAL, 	Compiler::NODE_GREATEREQUAL },
						{ Compiler::TOK_LESSTHAN, 		Compiler::NODE_LESSTHAN },
						{ Compiler::TOK_LESSEQUAL, 		Compiler::NODE_LESSEQUAL },
					};

					return parserState.AllocateNode< ComplexNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, map[ irBuilder.m_Token.m_Token ], left, nextExpression( irBuilder.m_Token.m_LBP ) );
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
			case Compiler::TOK_RETURN:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder )
				{
					return parserState.AllocateNode< SimpleNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_RETURN, nextExpression( irBuilder.m_Token.m_LBP ) );
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
			try
			{
				auto expression = nextExpression();
				parserState.AddNode( expression );

				if ( expression )
				{
					bool isExpressionStatement = true;

					switch ( expression->Id() )
					{
					case Compiler::NODE_NAME:
					{
						auto stringObject = AS_STRING( static_cast< ValueNode* >( expression )->GetValue() );
						isExpressionStatement = stringObject->GetString() != "print";
						break;
					}
					default:
						break;
					}

					if ( isExpressionStatement )
					{
						auto& token = parserState.CurrentBuilder()->m_Token;

						// Pop the intermediate value off stack
						parserState.AddNode( parserState.AllocateNode< TermNode >( token.m_LineNr,
							token.m_ColNr, token.m_String, NODE_POP ) );
					}
				}

				// A expression statements must end in a semicolon
				parserState.Expect( TOK_SCOLON, "Expected end of expression" );
			}
			catch ( const CompilerException& exception )
			{
				// A compilation error occurred, resync and continue parsing
				// to catch as many errors with a single pass as possible
				parserState.AddErrorAndResync( exception );
			}
		}

		// Exit interpreter loop on REPL mode
		parserState.AddNode( parserState.AllocateNode< TermNode >( -1, -1, "", NODE_RETURN ) );

		// If there were errors, throw them to the caller now
		if ( parserState.IsError() )
			throw parserState.Errors();

		// Builders are freed once parserState gets destructed
		return parserState.Product();
	}
}
