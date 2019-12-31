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

		auto nextStatement = [ &parserState, &nextExpression ]() -> BaseNode*
		{
			try
			{
				auto headNode = nextExpression();

				if ( headNode )
				{
					switch ( headNode->Id() )
					{
					case NODE_NAME:
					{
						throw CompilerException( "ir_unknown_symbol", "Unknown symbol \"" + headNode->Token() + "\"", headNode->LineNr(), headNode->ColNr(), headNode->Token() );
					}
					default:
						break;
					}

					if ( headNode->Id() == NODE_IF || headNode->Id() == NODE_WHILE )
					{
						auto currentToken = parserState.CurrentBuilder()->m_Token.m_Token;
						if ( currentToken != TOK_BRACE_RIGHT && currentToken != TOK_SCOLON )
						{
							auto builder = parserState.CurrentBuilder();
							throw CompilerException( "ir_expect", "Expected end of if/while-statement", builder->m_Token.m_LineNr,
								builder->m_Token.m_ColNr, builder->m_Token.m_String );
						}

						parserState.NextBuilder();
					}
					else
					{
						if ( headNode->Id() == NODE_SCOPE )
						{
							// Block declarations must end with a right brace
							parserState.Expect( TOK_BRACE_RIGHT, "Expected end of block declaration" );
						}
						else
						{
							// Expression statements must end with a semicolon
							parserState.Expect( TOK_SCOLON, "Expected end of expression" );
						}
					}
				}

				return headNode;
			}
			catch ( const CompilerException& exception )
			{
				// A compilation error occurred, resync and continue parsing
				// to catch as many errors with a single pass as possible
				parserState.AddErrorAndResync( exception );
				return NULL;
			}
		};

		auto createBuilder = [ &parserState, &nextExpression, &nextStatement ]( const Token_t& token ) -> IrBuilder_t*
		{
			IrBuilder_t* builder = new IrBuilder_t( token );

			switch ( token.m_Token )
			{
			case TOK_NULL:
			case TOK_FALSE:
			case TOK_TRUE:
			case TOK_DBL:
			case TOK_INT:
			case TOK_STR:
			{
				builder->m_Nud = [ &parserState ]( const IrBuilder_t& irBuilder )
				{
					QScript::Value value;

					switch ( irBuilder.m_Token.m_Token )
					{
						case TOK_STR:
							value.From( MAKE_STRING( irBuilder.m_Token.m_String ) );
							break;
						case TOK_INT:
							value.From( MAKE_NUMBER( ( double ) std::stoi( irBuilder.m_Token.m_String ) ) );
							break;
						case TOK_DBL:
							value.From( MAKE_NUMBER( std::stod( irBuilder.m_Token.m_String ) ) );
							break;
						case TOK_NULL:
							value.From( MAKE_NULL );
							break;
						case TOK_FALSE:
							value.From( MAKE_BOOL( false ) );
							break;
						case TOK_TRUE:
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
			case TOK_NAME:
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
			case TOK_VAR:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder )
				{
					auto varName = nextExpression( irBuilder.m_Token.m_LBP );

					if ( !varName->IsString() )
					{
						throw CompilerException( "ir_variable_name", "Invalid variable name: \"" + varName->Token() + "\"",
							varName->LineNr(), varName->ColNr(), varName->Token() );
					}

					if ( parserState.CurrentBuilder()->m_Token.m_Token == TOK_EQUALS )
					{
						parserState.NextBuilder();

						return parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
							irBuilder.m_Token.m_String, NODE_VAR, std::vector< BaseNode* >{ varName, nextExpression( BP_ASSIGN ) } );
					}
					else
					{
						return parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
							irBuilder.m_Token.m_String, NODE_VAR, std::vector< BaseNode* >{ varName, ( BaseNode* ) NULL } );
					}
				};
				break;
			}
			case TOK_BANG:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder )
				{
					return parserState.AllocateNode< SimpleNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_NOT, nextExpression( irBuilder.m_Token.m_LBP ) );
				};
				break;
			}
			case TOK_MINUS:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder )
				{
					return parserState.AllocateNode< ValueNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_NEG, nextExpression( -1 ) );
				};
				/* Fall Through */
			}
			case TOK_PLUS:
			case TOK_SLASH:
			case TOK_STAR:
			{
				builder->m_Led = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder, BaseNode* left )
				{
					std::map<Token, NodeId> map = {
						{ TOK_MINUS, 	NODE_SUB },
						{ TOK_STAR, 	NODE_MUL },
						{ TOK_SLASH, 	NODE_DIV },
						{ TOK_PLUS, 	NODE_ADD },
					};

					return parserState.AllocateNode< ComplexNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, map[ irBuilder.m_Token.m_Token ], left, nextExpression( irBuilder.m_Token.m_LBP ) );
				};
				break;
			}
			case TOK_EQUALS:
			{
				builder->m_Led = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder, BaseNode* left )
				{
					return parserState.AllocateNode< ComplexNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_ASSIGN, left, nextExpression( irBuilder.m_Token.m_LBP ) );
				};
				break;
			}
			case TOK_NOTEQUALS:
			case TOK_2EQUALS:
			case TOK_GREATERTHAN:
			case TOK_GREATEREQUAL:
			case TOK_LESSTHAN:
			case TOK_LESSEQUAL:
			case TOK_AND:
			case TOK_OR:
			{
				builder->m_Led = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder, BaseNode* left )
				{
					std::map<Token, NodeId> map = {
						{ TOK_2EQUALS, 			NODE_EQUALS },
						{ TOK_NOTEQUALS, 		NODE_NOTEQUALS },
						{ TOK_GREATERTHAN, 		NODE_GREATERTHAN },
						{ TOK_GREATEREQUAL, 	NODE_GREATEREQUAL },
						{ TOK_LESSTHAN, 		NODE_LESSTHAN },
						{ TOK_LESSEQUAL, 		NODE_LESSEQUAL },
						{ TOK_AND, 				NODE_AND },
						{ TOK_OR, 				NODE_OR },
					};

					return parserState.AllocateNode< ComplexNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, map[ irBuilder.m_Token.m_Token ], left, nextExpression( irBuilder.m_Token.m_LBP ) );
				};
				break;
			}
			case TOK_IF:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder ) -> BaseNode*
				{
					std::vector< BaseNode* > chain;

					// condition
					chain.push_back( nextExpression( irBuilder.m_Token.m_LBP ) );

					auto body = nextExpression( irBuilder.m_Token.m_LBP );
					if ( body->Id() != NODE_SCOPE )
					{
						body = parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
							irBuilder.m_Token.m_String, NODE_SCOPE, std::vector< BaseNode* >{ body } );
					}
					
					chain.push_back( body );

					// optional else block
					if ( parserState.Match( TOK_ELSE ) )
						chain.push_back( nextExpression( irBuilder.m_Token.m_LBP ) );
					else
						chain.push_back( NULL );

					return parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_IF, chain );
				};
				break;
			}
			case TOK_ELSE:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder ) -> BaseNode*
				{
					return nextExpression( irBuilder.m_Token.m_LBP );
				};
				break;
			}
			case TOK_WHILE:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder ) -> BaseNode*
				{
					auto condition = nextExpression( irBuilder.m_Token.m_LBP );

					auto body = nextExpression( irBuilder.m_Token.m_LBP );
					if ( body->Id() != NODE_SCOPE )
					{
						body = parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
							irBuilder.m_Token.m_String, NODE_SCOPE, std::vector< BaseNode* >{ body } );
					}

					return parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_WHILE, std::vector< BaseNode* >{ condition, body } );
				};
				break;
			}
			case TOK_DO:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder ) -> BaseNode*
				{
					auto body = nextExpression( irBuilder.m_Token.m_LBP );
					if ( body->Id() != NODE_SCOPE )
					{
						body = parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
							irBuilder.m_Token.m_String, NODE_SCOPE, std::vector< BaseNode* >{ body } );
					}

					// Skip over terminating token (either ; or })
					parserState.NextBuilder();

					parserState.Expect( TOK_WHILE, "Expected \"while\" after do <block>, got: \"" + parserState.CurrentBuilder()->m_Token.m_String + "\"" );

					auto condition = nextExpression( irBuilder.m_Token.m_LBP );

					return parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_DO, std::vector< BaseNode* >{ body, condition } );
				};
				break;
			}
			case TOK_BRACE_LEFT:
			{
				builder->m_Nud = [ &parserState, &nextStatement ]( const IrBuilder_t& irBuilder ) -> BaseNode*
				{
					std::vector< BaseNode* > scopeExpressions;

					while ( !parserState.IsFinished() && parserState.CurrentBuilder()->m_Token.m_Token != TOK_BRACE_RIGHT )
					{
						auto headNode = nextStatement();

						if ( headNode )
							scopeExpressions.push_back( headNode );
					}

					return parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_SCOPE, scopeExpressions );
				};
				break;
			}
			case TOK_BRACE_RIGHT:
			case TOK_RPAREN:
			case TOK_SCOLON:
				break;
			case TOK_LPAREN:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder )
				{
					// TODO: check for instant closing rparen

					auto expression = nextExpression();

					if ( parserState.NextBuilder()->m_Token.m_Token != TOK_RPAREN )
					{
						auto curBuilder = parserState.CurrentBuilder();
						throw Exception( "ir_missing_rparen",
							std::string( "Expected an end of expression, got: \"" + curBuilder->m_Token.m_String + "\"" ) );
					}

					return expression;
				};
				break;
			}
			case TOK_RETURN:
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
			auto headNode = nextStatement();

			if ( headNode )
				parserState.AddNode( headNode );
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
