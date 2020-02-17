#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Compiler.h"
#include "Instructions.h"
#include "IR.h"

namespace Compiler
{
	bool IsString( BaseNode* node )
	{
		if ( node->Type() != NT_VALUE )
			return false;

		return IS_STRING( static_cast< ValueNode* >( node )->GetValue() );
	}

	CompileTypeInfo ResolveTypeDef( ParserState& parserState )
	{
		std::map< Token, CompileTypeInfo > typeMap = {
			{ TOK_AUTO, TYPE_AUTO },
			{ TOK_STRING, TYPE_STRING },
			{ TOK_BOOL, TYPE_BOOL },
			{ TOK_NUMBER, TYPE_NUMBER },
			{ TOK_VAR, TYPE_UNKNOWN },
		};

		for ( auto type : typeMap )
		{
			if ( parserState.MatchCurrent( type.first ) )
				return type.second;
		}

		return TYPE_UNKNOWN;
	}

	bool IsFunctionDefinition( ParserState& parserState )
	{
		int offset = parserState.Offset();
		IrBuilder_t* builder = parserState.Peek( offset );

		for ( ; builder; offset += 1, builder = parserState.Peek( offset ) )
		{
			switch ( builder->m_Token.m_Id )
			{
			case Compiler::TOK_COMMA:
			case Compiler::TOK_NAME:
				continue;
			case Compiler::TOK_PAREN_RIGHT:
				break;
			default:
				return false;
			}

			break;
		}

		if ( builder == NULL )
			return false;

		auto arrowToken = parserState.Peek( ++offset );

		if ( !arrowToken || arrowToken->m_Token.m_Id != Compiler::TOK_ARROW )
			return false;

		return true;
	}

	void EndStatement( const BaseNode* node, ParserState& parserState )
	{
		if ( node && node->Id() == NODE_FUNC )
		{
			// Don't mandate a semicolon after functions
			parserState.Expect( TOK_BRACE_RIGHT, "Expected end of block declaration" );
			parserState.MatchCurrent( TOK_SCOLON );
		}
		else
		{
			// Expression and statements must end with a semicolon
			parserState.Expect( TOK_SCOLON, "Expected end of expression" );
		}
	}

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

				if ( parserState.IsFinished() )
					throw CompilerException( "ir_parsing_past_eof", "Parsing past end of file", -1, -1, "" );

				if ( builder->m_Led == NULL )
				{
					throw CompilerException( "ir_expect_rvalue", "Expected a right-value",
						builder->m_Token.m_LineNr, builder->m_Token.m_ColNr, builder->m_Token.m_String );
				}

				left = builder->m_Led( *builder, left );

				if ( parserState.IsFinished() )
					throw CompilerException( "ir_parsing_past_eof", "Parsing past end of file", -1, -1, "" );
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

					switch ( headNode->Id() )
					{
					case NODE_CLASS:
					{
						parserState.Expect( TOK_BRACE_RIGHT, "Expected end of class declaration" );

						// Allow trailing semicolon on class definitions
						parserState.MatchCurrent( TOK_SCOLON );
						break;
					}
					case NODE_FUNC:
					{
						// Allow trailing semicolon on function definitions
						parserState.MatchCurrent( TOK_SCOLON );
						break;
					}
					case NODE_ASSIGN:
					{
						auto assignNode = static_cast< ComplexNode* >( headNode );
						EndStatement( assignNode->GetRight(), parserState );
						break;
					}
					case NODE_CONSTVAR:
					case NODE_VAR:
					{
						auto varNode = static_cast< ListNode* >( headNode );
						EndStatement( varNode->GetList()[ 1 ], parserState );
						break;
					}
					case NODE_RETURN:
					{
						auto returnNode = static_cast< SimpleNode* >( headNode );
						EndStatement( returnNode->GetNode(), parserState );
						break;
					}
					case NODE_IF:
					case NODE_WHILE:
					case NODE_FOR:
					{
						auto currentToken = parserState.CurrentBuilder()->m_Token.m_Id;
						if ( currentToken != TOK_BRACE_RIGHT && currentToken != TOK_SCOLON )
						{
							auto builder = parserState.CurrentBuilder();
							throw CompilerException( "ir_expect", "Expected end of if/for/while-statement", builder->m_Token.m_LineNr,
								builder->m_Token.m_ColNr, builder->m_Token.m_String );
						}

						parserState.NextBuilder();
						break;
					}
					default:
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
						break;
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
			IrBuilder_t* builder = QS_NEW IrBuilder_t( token );

			switch ( token.m_Id )
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

					switch ( irBuilder.m_Token.m_Id )
					{
						case TOK_STR:
							value = MAKE_STRING( irBuilder.m_Token.m_String );
							break;
						case TOK_INT:
							value = MAKE_NUMBER( ( double ) std::stoi( irBuilder.m_Token.m_String ) );
							break;
						case TOK_DBL:
							value = MAKE_NUMBER( std::stod( irBuilder.m_Token.m_String ) );
							break;
						case TOK_NULL:
							value = MAKE_NULL;
							break;
						case TOK_FALSE:
							value = MAKE_BOOL( false );
							break;
						case TOK_TRUE:
							value = MAKE_BOOL( true );
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
					return parserState.AllocateNode< ValueNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_NAME, MAKE_STRING( irBuilder.m_Token.m_String ) );
				};
				break;
			}
			case TOK_AUTO:
			case TOK_BOOL:
			case TOK_STRING:
			case TOK_NUMBER:
			case TOK_CONST:
			case TOK_VAR:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder )
				{
					auto varType = TYPE_UNKNOWN;

					switch ( irBuilder.m_Token.m_Id )
					{
					case TOK_AUTO:
						varType = TYPE_AUTO;
						break;
					case TOK_BOOL:
						varType = TYPE_BOOL;
						break;
					case TOK_STRING:
						varType = TYPE_STRING;
						break;
					case TOK_NUMBER:
						varType = TYPE_NUMBER;
						break;
					case TOK_CONST:
						varType = ResolveTypeDef( parserState );
						break;
					case TOK_VAR:
					default:
						break;
					}

					auto varName = nextExpression( irBuilder.m_Token.m_LBP );

					if ( !IsString( varName ) )
					{
						throw CompilerException( "ir_variable_name", "Invalid variable name: \"" + varName->Token() + "\"",
							varName->LineNr(), varName->ColNr(), varName->Token() );
					}

					// Append type information as a value node
					auto varTypeNode = parserState.AllocateNode< ValueNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_CONSTANT, MAKE_NUMBER( varType ) );

					if ( parserState.CurrentBuilder()->m_Token.m_Id == TOK_EQUALS )
					{
						parserState.NextBuilder();

						return parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
							irBuilder.m_Token.m_String, irBuilder.m_Token.m_Id == TOK_CONST ? NODE_CONSTVAR : NODE_VAR,
							std::vector< BaseNode* >{ varName, nextExpression( BP_ASSIGN ), varTypeNode } );
					}
					else
					{
						return parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
							irBuilder.m_Token.m_String,  irBuilder.m_Token.m_Id == TOK_CONST ? NODE_CONSTVAR : NODE_VAR,
							std::vector< BaseNode* >{ varName, ( BaseNode* ) NULL, varTypeNode } );
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
					return parserState.AllocateNode< SimpleNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_NEG, nextExpression( -1 ) );
				};
				/* Fall Through */
			}
			case TOK_PLUS:
			case TOK_SLASH:
			case TOK_STAR:
			case TOK_PERCENT:
			case TOK_2STAR:
			{
				builder->m_Led = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder, BaseNode* left )
				{
					std::map<Token, NodeId> map = {
						{ TOK_MINUS, 	NODE_SUB },
						{ TOK_STAR, 	NODE_MUL },
						{ TOK_SLASH, 	NODE_DIV },
						{ TOK_PLUS, 	NODE_ADD },
						{ TOK_2STAR, 	NODE_POW },
						{ TOK_PERCENT, 	NODE_MOD },
					};

					return parserState.AllocateNode< ComplexNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, map[ irBuilder.m_Token.m_Id ], left, nextExpression( irBuilder.m_Token.m_LBP ) );
				};
				break;
			}
			case TOK_EQUALS:
			case TOK_EQUALSADD:
			case TOK_EQUALSDIV:
			case TOK_EQUALSSUB:
			case TOK_EQUALSMUL:
			case TOK_EQUALSMOD:
			{
				builder->m_Led = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder, BaseNode* left )
				{
					std::map<Token, NodeId> map = {
						{ TOK_EQUALS, 		NODE_ASSIGN },
						{ TOK_EQUALSADD, 	NODE_ASSIGNADD },
						{ TOK_EQUALSDIV, 	NODE_ASSIGNDIV },
						{ TOK_EQUALSSUB, 	NODE_ASSIGNSUB },
						{ TOK_EQUALSMUL, 	NODE_ASSIGNMUL },
						{ TOK_EQUALSMOD, 	NODE_ASSIGNMOD },
					};

					return parserState.AllocateNode< ComplexNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, map[ irBuilder.m_Token.m_Id ], left, nextExpression( irBuilder.m_Token.m_LBP ) );
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
						irBuilder.m_Token.m_String, map[ irBuilder.m_Token.m_Id ], left, nextExpression( irBuilder.m_Token.m_LBP ) );
				};
				break;
			}
			case TOK_2PLUS:
			case TOK_2MINUS:
			{
				builder->m_Led = [ &parserState ]( const IrBuilder_t& irBuilder, BaseNode* left ) -> BaseNode*
				{
					return parserState.AllocateNode< ComplexNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, irBuilder.m_Token.m_Id == TOK_2PLUS ? NODE_INC : NODE_DEC,
						left, ( BaseNode* ) NULL );
				};

				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder ) -> BaseNode*
				{
					return parserState.AllocateNode< ComplexNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, irBuilder.m_Token.m_Id == TOK_2PLUS ? NODE_INC : NODE_DEC,
						( BaseNode* ) NULL, nextExpression( irBuilder.m_Token.m_LBP ) );
				};
				break;
			}
			case TOK_IMPORT:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder )
				{
					auto moduleName = nextExpression( BP_VAR );

					if ( !IsString( moduleName ) )
					{
						throw CompilerException( "ir_invalid_import",
							"Invalid import target \"" + moduleName->Token() + "\"",
							moduleName->LineNr(), moduleName->ColNr(), moduleName->Token() );
					}

					return parserState.AllocateNode< SimpleNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_IMPORT, moduleName );
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

					auto body = parserState.ToScope( nextExpression( irBuilder.m_Token.m_LBP ) );
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

					auto body = parserState.ToScope( nextExpression( irBuilder.m_Token.m_LBP ) );

					return parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_WHILE, std::vector< BaseNode* >{ condition, body } );
				};
				break;
			}
			case TOK_DO:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder ) -> BaseNode*
				{
					auto body = parserState.ToScope( nextExpression( irBuilder.m_Token.m_LBP ) );

					// Skip over terminating token (either ; or })
					parserState.NextBuilder();

					parserState.Expect( TOK_WHILE, "Expected \"while\" after \"do <block>\", got: \"" + parserState.CurrentBuilder()->m_Token.m_String + "\"" );

					auto condition = nextExpression( irBuilder.m_Token.m_LBP );

					return parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_DO, std::vector< BaseNode* >{ body, condition } );
				};
				break;
			}
			case TOK_FOR:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder ) -> BaseNode*
				{
					std::vector< BaseNode* > forStatement;

					parserState.Expect( TOK_PAREN_LEFT, "Expected \"(\" after \"for\", got: \"" + parserState.CurrentBuilder()->m_Token.m_String + "\"" );

					for ( int i = 0; i < 4; ++i )
					{
						if ( parserState.MatchCurrent( TOK_PAREN_RIGHT ) )
						{
							if ( forStatement.size() == 0 )
							{
								auto builder = parserState.CurrentBuilder();
								throw CompilerException( "ir_empty_forloop", "Empty forloop. Expected a list of expressions", builder->m_Token.m_LineNr,
									builder->m_Token.m_ColNr, builder->m_Token.m_String );
							}

							// If the last parsed expression was null, add a NULL for the increment clause as well
							if ( forStatement[ forStatement.size() - 1 ] == NULL )
								forStatement.push_back( NULL );

							// If only 2 expressions were parsed, add a NULL for the increment clause as well
							if ( forStatement.size() == 2 )
								forStatement.push_back( NULL );

							break;
						}
						else if ( parserState.MatchCurrent( TOK_SCOLON ) )
						{
							forStatement.push_back( NULL );
						}
						else
						{
							forStatement.push_back( nextExpression( irBuilder.m_Token.m_LBP ) );

							if ( i < 2 )
								parserState.Expect( TOK_SCOLON, "Expected end of expression" );
						}
					}

					auto body = parserState.ToScope( nextExpression( irBuilder.m_Token.m_LBP ) );
					forStatement.push_back( body );

					return parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_FOR, forStatement );
				};
				break;
			}
			case TOK_CLASS:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder ) -> BaseNode*
				{
					auto className = nextExpression( BP_VAR );

					if ( !IsString( className ) )
					{
						throw CompilerException( "ir_class_name", "Invalid class name: \"" + className->Token() + "\"",
							className->LineNr(), className->ColNr(), className->Token() );
					}

					parserState.Expect( TOK_BRACE_LEFT, "Expected \"{\" before class body, got: \"" + parserState.CurrentBuilder()->m_Token.m_String + "\"" );

					return parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_CLASS, std::vector< BaseNode* >{ className } );
				};
				break;
			}
			case TOK_DOT:
			{
				builder->m_Led = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder, BaseNode* left ) -> BaseNode*
				{
					auto propName = nextExpression( irBuilder.m_Token.m_LBP );

					if ( !IsString( propName ) )
					{
						throw CompilerException( "ir_property_name", "Invalid property name: \"" + propName->Token() + "\"",
							propName->LineNr(), propName->ColNr(), propName->Token() );
					}

					return parserState.AllocateNode< ComplexNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_ACCESS_PROP, left, propName );
				};
				break;
			}
			case TOK_BRACE_LEFT:
			{
				builder->m_Nud = [ &parserState, &nextStatement ]( const IrBuilder_t& irBuilder ) -> BaseNode*
				{
					std::vector< BaseNode* > scopeExpressions;

					while ( !parserState.IsFinished() && parserState.CurrentBuilder()->m_Token.m_Id != TOK_BRACE_RIGHT )
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
			case TOK_PAREN_RIGHT:
			case TOK_SCOLON:
			case TOK_COMMA:
			case TOK_ARROW:
				break;
			case TOK_PAREN_LEFT:
			{
				builder->m_Nud = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder ) -> BaseNode*
				{
					if ( IsFunctionDefinition( parserState ) )
					{
						std::vector< BaseNode* > argsList;

						if ( !parserState.MatchCurrent( TOK_PAREN_RIGHT ) )
						{
							do {
								auto argName = nextExpression( BP_VAR );

								if ( !IsString( argName ) )
								{
									throw CompilerException( "ir_variable_name", "Invalid variable name: \"" + argName->Token() + "\"",
										argName->LineNr(), argName->ColNr(), argName->Token() );
								}

								argsList.push_back( argName );
							} while ( parserState.MatchCurrent( TOK_COMMA ) );

							parserState.Expect( TOK_PAREN_RIGHT, "Expected \")\" after \"var <name> = (...\", got: \"" + parserState.CurrentBuilder()->m_Token.m_String + "\"" );
						}

						// Skip over arrow
						parserState.Expect( TOK_ARROW, "Expected \"->\" after \"var <name> = (...)\", got: \"" + parserState.CurrentBuilder()->m_Token.m_String + "\"" );

						// Check for explicit return type
						uint32_t retnType = ResolveTypeDef( parserState );

						// Append type information as a value node
						auto retnTypeNode = parserState.AllocateNode< ValueNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
							irBuilder.m_Token.m_String, NODE_CONSTANT, MAKE_NUMBER( retnType ) );

						auto body = parserState.ToScope( nextExpression( irBuilder.m_Token.m_LBP ) );

						auto args = parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
								irBuilder.m_Token.m_String, NODE_ARGUMENTS, argsList );

						return parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
							irBuilder.m_Token.m_String, NODE_FUNC, std::vector< BaseNode* >{ args, body, retnTypeNode } );
					}
					else
					{
						auto expression = nextExpression();

						if ( parserState.NextBuilder()->m_Token.m_Id != TOK_PAREN_RIGHT )
						{
							auto curBuilder = parserState.CurrentBuilder();
							throw Exception( "ir_missing_rparen",
								std::string( "Expected an end of expression, got: \"" + curBuilder->m_Token.m_String + "\"" ) );
						}

						return expression;
					}
				};

				builder->m_Led = [ &parserState, &nextExpression ]( const IrBuilder_t& irBuilder, BaseNode* left )
				{
					std::vector< BaseNode* > argsList;

					// Parse argument list
					if ( !parserState.MatchCurrent( TOK_PAREN_RIGHT ) )
					{
						do {
							argsList.push_back( nextExpression() );
						} while ( parserState.MatchCurrent( TOK_COMMA ) );

						parserState.Expect( TOK_PAREN_RIGHT, "Expected \")\" after function call, got: \"" + parserState.CurrentBuilder()->m_Token.m_String + "\"" );
					}

					auto args = parserState.AllocateNode< ListNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_ARGUMENTS, argsList );

					return parserState.AllocateNode< ComplexNode >( irBuilder.m_Token.m_LineNr, irBuilder.m_Token.m_ColNr,
						irBuilder.m_Token.m_String, NODE_CALL, left, args );
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
					+ std::to_string( token.m_Id )
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
