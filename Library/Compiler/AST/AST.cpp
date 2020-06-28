#include "QLibPCH.h"
#include "Instructions.h"
#include "Typing.h"

#include "../../Common/Chunk.h"
#include "../../STL/NativeModule.h"
#include "../Compiler.h"

#define COMPILE_EXPRESSION( options ) (options | CO_EXPRESSION)
#define COMPILE_STATEMENT( options ) (options & ~CO_EXPRESSION)
#define COMPILE_ASSIGN_TARGET( options ) (options | CO_ASSIGN)
#define COMPILE_REASSIGN_TARGET( options ) (options | CO_ASSIGN | CO_REASSIGN)
#define COMPILE_NON_ASSIGN( options ) (options & ~(CO_ASSIGN | CO_REASSIGN))

#define IS_STATEMENT( options ) (!(options & CO_EXPRESSION))
#define IS_ASSIGN_TARGET( options ) ((options & (CO_ASSIGN | CO_REASSIGN)))

#define EXPECTED_EXPRESSION CompilerException( "cp_expected_expression", "Expected an expression, got: \"" + m_Token + "\" (statement)", m_LineNr, m_ColNr, m_Token );
#define EXPECTED_STATEMENT CompilerException( "cp_expected_statement", "Expected a statement, got: \"" + m_Token + "\" (expression)", m_LineNr, m_ColNr, m_Token );

namespace Compiler
{
	void AddDebugSymbol( Compiler::Assembler& assembler, uint32_t start, int lineNr, int colNr, const std::string token )
	{
		if ( !assembler.Config().m_DebugSymbols )
			return;

		if ( lineNr == -1 )
			return;

		auto chunk = assembler.CurrentChunk();
		uint32_t end = ( uint32_t ) chunk->m_Code.size();

		if ( start == end )
			return;

		chunk->m_Debug.push_back( QScript::Chunk_t::Debug_t{
			start,
			end,
			lineNr,
			colNr,
			token,
		} );
	}

	uint32_t EmitConstant( QScript::Chunk_t* chunk, const QScript::Value& value, QScript::OpCode shortOpCode, QScript::OpCode longOpCode, Compiler::Assembler& assembler )
	{
		uint32_t constant = AddConstant( value, chunk );

		if ( constant > 255 )
		{
			EmitByte( longOpCode, chunk );
			EmitByte( ENCODE_LONG( constant, 0 ), chunk );
			EmitByte( ENCODE_LONG( constant, 1 ), chunk );
			EmitByte( ENCODE_LONG( constant, 2 ), chunk );
			EmitByte( ENCODE_LONG( constant, 3 ), chunk );
		}
		else
		{
			EmitByte( shortOpCode, chunk );
			EmitByte( ( uint8_t ) constant, chunk );
		}

		return constant;
	}

	void RelocateDebugSymbols( QScript::Chunk_t* chunk, uint32_t from, uint32_t patchSize )
	{
		for ( auto& symbol : chunk->m_Debug )
		{
			if ( symbol.m_From > from )
				symbol.m_From += patchSize;

			if ( symbol.m_To > from )
				symbol.m_To += patchSize;
		}
	}

	uint32_t PlaceJump( QScript::Chunk_t* chunk, uint32_t from, uint32_t size, QScript::OpCode shortOpCode, QScript::OpCode longOpCode )
	{
		bool isLong = size > 255;
		short patchSize = isLong ? 5 : 2;

		std::vector< uint8_t > body;
		body.insert( body.begin(), chunk->m_Code.begin() + from, chunk->m_Code.end() );

		// Append patchSize additional bytes for jump
		chunk->m_Code.resize( chunk->m_Code.size() + patchSize, 0xFF );

		// Move body by patchSize
		for ( size_t i = 0; i < body.size(); ++i )
			chunk->m_Code[ from + i + patchSize ] = body[ i ];

		if ( isLong )
		{
			// Write long jump
			chunk->m_Code[ from ] = longOpCode;
			chunk->m_Code[ from + 1 ] = ENCODE_LONG( size, 0 );
			chunk->m_Code[ from + 2 ] = ENCODE_LONG( size, 1 );
			chunk->m_Code[ from + 3 ] = ENCODE_LONG( size, 2 );
			chunk->m_Code[ from + 4 ] = ENCODE_LONG( size, 3 );
		}
		else
		{
			// Patch single-byte jump
			chunk->m_Code[ from ] = shortOpCode;
			chunk->m_Code[ from + 1 ] = ( uint8_t ) size;
		}

		RelocateDebugSymbols( chunk, from, patchSize );

		return patchSize;
	}

	void PatchJump( QScript::Chunk_t* chunk, uint32_t from, uint32_t newSize, QScript::OpCode shortOpCode, QScript::OpCode longOpCode )
	{
		if ( chunk->m_Code[ from ] == shortOpCode )
		{
			if ( newSize > 255 )
				throw Exception( "cp_invalid_jmp_patch_size", "Expected long JMP instruction, got short" );

			chunk->m_Code[ from + 1 ] = ( uint8_t ) newSize;
		}
		else if ( chunk->m_Code[ from ] == longOpCode )
		{
			chunk->m_Code[ from + 1 ] = ENCODE_LONG( newSize, 0 );
			chunk->m_Code[ from + 2 ] = ENCODE_LONG( newSize, 1 );
			chunk->m_Code[ from + 3 ] = ENCODE_LONG( newSize, 2 );
			chunk->m_Code[ from + 4 ] = ENCODE_LONG( newSize, 3 );
		}
		else
		{
			throw Exception( "cp_invalid_jmp_patch_inst", "Expected JMP instruction at patch site" );
		}
	}

	void RequireAssignability( BaseNode* node )
	{
		switch ( node->Id() )
		{
		case NODE_NAME:
		case NODE_ACCESS_PROP:
		case NODE_ACCESS_ARRAY:
			break;
		default:
			throw CompilerException( "cp_invalid_assign_target", "Invalid assignment target", node->LineNr(), node->ColNr(), node->Token() );
		}
	}

	std::vector< Argument_t > ParseArgsList( ListNode* argNode )
	{
		std::vector< Argument_t > argsList;

		for ( auto arg : argNode->GetList() )
		{
			switch ( arg->Id() )
			{
			case NODE_VAR:
			{
				auto listNode = static_cast< ListNode* >( arg );
				auto varNameNode = listNode->GetList()[ 0 ];
				auto varTypeNode = listNode->GetList()[ 2 ];

				auto varName = static_cast< ValueNode* >( varNameNode )->GetValue();
				uint32_t varType = ( uint32_t ) AS_NUMBER( static_cast< ValueNode* >( varTypeNode )->GetValue() );

				switch ( varType )
				{
				case TYPE_NUMBER:
				case TYPE_STRING:
				case TYPE_UNKNOWN:
				case TYPE_BOOL:
				{
					argsList.push_back( Argument_t{ AS_STRING( varName )->GetString(), varType, varNameNode->LineNr(), varNameNode->ColNr() } );
					break;
				}
				default:
				{
					throw CompilerException( "cp_invalid_function_arg_type", "Invalid argument type: " + TypeToString( varType ),
						arg->LineNr(), arg->ColNr(), arg->Token() );
				}
				}
				break;
			}
			case NODE_NAME:
			{
				argsList.push_back( Argument_t{ AS_STRING( static_cast< ValueNode* >( arg )->GetValue() )->GetString(),
					TYPE_UNKNOWN, arg->LineNr(), arg->ColNr() } );
				break;
			}
			default:
			{
				throw CompilerException( "cp_invalid_function_arg", "Unknown argument node: " + std::to_string( arg->Id() ),
					arg->LineNr(), arg->ColNr(), arg->Token() );
			}
			}
		}

		return argsList;
	}

	QScript::FunctionObject* CompileFunction( bool isAnonymous, bool isConst, bool isMember, const std::string& name, ListNode* funcNode,
		Assembler& assembler, Type_t* outReturnType = NULL, int lineNr = -1, int colNr = -1 )
	{
		auto chunk = assembler.CurrentChunk();
		auto& nodeList = funcNode->GetList();

		auto argNode = static_cast< ListNode* >( nodeList[ 0 ] );
		auto returnType = ResolveReturnType( funcNode, assembler );

		// Allocate chunk & create function
		auto function = assembler.CreateFunction( name, isConst, returnType, isAnonymous, !isMember, QScript::AllocChunk() );

		if ( outReturnType )
			*outReturnType = returnType;

		if ( isMember )
			assembler.AddLocal( "this", false, -1, -1, TYPE_TABLE );

		assembler.PushScope();

		// Create args in scope
		auto argsList = ParseArgsList( argNode );
		std::for_each( argsList.begin(), argsList.end(), [ &assembler, &function ]( const Argument_t& item ) {
			function->AddArgument( item.m_Name, item.m_Type );
			assembler.AddLocal( item.m_Name, true, item.m_LineNr, item.m_ColNr, item.m_Type );
		} );

		// Compile function body
		for ( auto node : static_cast< ListNode* >( nodeList[ 1 ] )->GetList() )
			node->Compile( assembler, COMPILE_STATEMENT( 0 ) );

		std::vector< Assembler::Upvalue_t > upvalues;

		// Remove body scope
		assembler.FinishFunction( lineNr, colNr, &upvalues );

		// Create a constant (function) in enclosing chunk
		EmitConstant( chunk, MAKE_OBJECT( function ), QScript::OpCode::OP_CLOSURE_SHORT,
			QScript::OpCode::OP_CLOSURE_LONG, assembler );

		// Emit upvalues
		for ( auto upvalue : upvalues )
		{
			EmitByte( upvalue.m_IsLocal ? 1 : 0, chunk );
			EmitByte( ENCODE_LONG( upvalue.m_Index, 0 ), chunk );
			EmitByte( ENCODE_LONG( upvalue.m_Index, 1 ), chunk );
			EmitByte( ENCODE_LONG( upvalue.m_Index, 2 ), chunk );
			EmitByte( ENCODE_LONG( upvalue.m_Index, 3 ), chunk );
		}

		return function;
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

	void TermNode::Compile( Assembler& assembler, uint32_t options )
	{
		auto chunk = assembler.CurrentChunk();

		// Call CompileXXX functions from Compiler.cpp for code generation,
		// but append debugging info to the chunk here
		uint32_t start = ( uint32_t ) chunk->m_Code.size();

		switch( m_NodeId )
		{
		case NODE_RETURN:
		{
			// Check that the function can return null
			auto& type = assembler.CurrentContext()->m_Type;
			auto retnType = type.m_ReturnType ? *type.m_ReturnType : Type_t( TYPE_UNKNOWN );

			if ( !retnType.IsUnknown() && !( retnType & TYPE_NULL ) )
			{
				throw CompilerException( "cp_invalid_expression_type", "Expected expression of type " +
					TypeToString( retnType.m_Bits ) + ", got: " + TypeToString( TYPE_NULL ),
					LineNr(), ColNr(), Token() );
			}

			EmitByte( QScript::OpCode::OP_LOAD_NULL, chunk );
			EmitByte( QScript::OpCode::OP_RETURN, chunk );
			break;
		}
		default:
			throw CompilerException( "cp_invalid_term_node", "Unknown terminating node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}

		AddDebugSymbol( assembler, start, m_LineNr, m_ColNr, m_Token );
	}

	ValueNode::ValueNode( int lineNr, int colNr, const std::string token, NodeId id, const QScript::Value& value )
		: BaseNode( lineNr, colNr, token, NT_VALUE, id )
	{
		m_Value = value;
	}

	void ValueNode::Compile( Assembler& assembler, uint32_t options )
	{
		auto chunk = assembler.CurrentChunk();
		auto& config = assembler.Config();
		uint32_t start = ( uint32_t ) chunk->m_Code.size();

		switch ( m_NodeId )
		{
		case NODE_NAME:
		{
			uint32_t nameIndex = 0;
			auto name = AS_STRING( m_Value )->GetString();

			bool canEmit = false;
			QScript::OpCode opCodeShort = QScript::OpCode::OP_NOP;
			QScript::OpCode opCodeLong = QScript::OpCode::OP_NOP;
			QScript::OpCode opCodeShortAssign = QScript::OpCode::OP_NOP;
			QScript::OpCode opCodeLongAssign = QScript::OpCode::OP_NOP;

			Variable_t varInfo;
			if ( assembler.FindLocal( name, &nameIndex, &varInfo ) )
			{
				if ( IS_ASSIGN_TARGET( options ) )
				{
					if ( ( options & CO_REASSIGN ) && varInfo.m_IsConst )
					{
						throw CompilerException( "cp_assign_to_const", "Assigning to constant variable: \"" + varInfo.m_Name + "\"",
							m_LineNr, m_ColNr, m_Token );
					}

					if ( nameIndex < QScript::OP_SET_LOCAL_MAX )
						EmitByte( QScript::OP_SET_LOCAL_0 + ( uint8_t ) nameIndex, chunk );
					else
						canEmit = true;
				}
				else
				{
					if ( nameIndex < QScript::OP_LOAD_LOCAL_MAX )
						EmitByte( QScript::OP_LOAD_LOCAL_0 + ( uint8_t ) nameIndex, chunk );
					else
						canEmit = true;
				}

				opCodeShort = QScript::OpCode::OP_LOAD_LOCAL_SHORT;
				opCodeLong = QScript::OpCode::OP_LOAD_LOCAL_LONG;
				opCodeShortAssign = QScript::OpCode::OP_SET_LOCAL_SHORT;
				opCodeLongAssign = QScript::OpCode::OP_SET_LOCAL_LONG;

				if ( config.m_IdentifierCb )
					config.m_IdentifierCb( m_LineNr, m_ColNr, varInfo, "Local" );
			}
			else if ( assembler.RequestUpvalue( name, &nameIndex, m_LineNr, m_ColNr, &varInfo ) )
			{
				if ( ( options & CO_REASSIGN ) && varInfo.m_IsConst )
				{
					throw CompilerException( "cp_assign_to_const", "Assigning to constant variable: \"" + varInfo.m_Name + "\"",
						m_LineNr, m_ColNr, m_Token );
				}

				canEmit = true;
				opCodeShort = QScript::OpCode::OP_LOAD_UPVALUE_SHORT;
				opCodeLong = QScript::OpCode::OP_LOAD_UPVALUE_LONG;
				opCodeShortAssign = QScript::OpCode::OP_SET_UPVALUE_SHORT;
				opCodeLongAssign = QScript::OpCode::OP_SET_UPVALUE_LONG;

				if ( config.m_IdentifierCb )
					config.m_IdentifierCb( m_LineNr, m_ColNr, varInfo, "Upvalue" );
			}
			else if ( assembler.FindGlobal( name, &varInfo ) )
			{
				if ( IS_ASSIGN_TARGET( options ) )
				{
					if ( ( options & CO_REASSIGN ) && varInfo.m_IsConst )
					{
						throw CompilerException( "cp_assign_to_const", "Assigning to constant variable: \"" + varInfo.m_Name + "\"",
							m_LineNr, m_ColNr, m_Token );
					}

					EmitConstant( chunk, m_Value, QScript::OpCode::OP_SET_GLOBAL_SHORT, QScript::OpCode::OP_SET_GLOBAL_LONG, assembler );
				}
				else
				{
					EmitConstant( chunk, m_Value, QScript::OpCode::OP_LOAD_GLOBAL_SHORT, QScript::OpCode::OP_LOAD_GLOBAL_LONG, assembler );
				}

				if ( config.m_IdentifierCb )
					config.m_IdentifierCb( m_LineNr, m_ColNr, varInfo, "Global" );
			}
			else
			{
				throw CompilerException( "cp_unknown_identifier", "Referenced an unknown identifier \"" + name + "\"",
					m_LineNr, m_ColNr, m_Token );
			}

			if ( canEmit )
			{
				if ( nameIndex > 255 )
				{
					EmitByte( IS_ASSIGN_TARGET( options ) ? opCodeLongAssign : opCodeLong, chunk );
					EmitByte( ENCODE_LONG( nameIndex, 0 ), chunk );
					EmitByte( ENCODE_LONG( nameIndex, 1 ), chunk );
					EmitByte( ENCODE_LONG( nameIndex, 2 ), chunk );
					EmitByte( ENCODE_LONG( nameIndex, 3 ), chunk );
				}
				else
				{
					EmitByte( IS_ASSIGN_TARGET( options ) ? opCodeShortAssign : opCodeShort, chunk );
					EmitByte( ( uint8_t ) nameIndex, chunk );
				}
			}

			break;
		}
		case NODE_CONSTANT:
		{
			if ( IS_STATEMENT( options ) )
				throw EXPECTED_STATEMENT;

			bool fastLoad = false;

			if ( IS_NUMBER( m_Value ) )
			{
				auto number = AS_NUMBER( m_Value );

				if ( std::floor( number ) == number && number >= -1 && number < QScript::OP_LOAD_MAX )
				{
					fastLoad = true;
					EmitByte( QScript::OP_LOAD_0 + ( uint8_t ) number, chunk );
				}
			}
			else if ( IS_NULL( m_Value ) )
			{
				fastLoad = true;
				EmitByte( QScript::OP_LOAD_NULL, chunk );
			}

			if ( !fastLoad )
				EmitConstant( chunk, m_Value, QScript::OpCode::OP_LOAD_CONSTANT_SHORT, QScript::OpCode::OP_LOAD_CONSTANT_LONG, assembler );

			break;
		}
		default:
			throw CompilerException( "cp_invalid_value_node", "Unknown value node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}

		AddDebugSymbol( assembler, start, m_LineNr, m_ColNr, m_Token );
	}

	ComplexNode::ComplexNode( int lineNr, int colNr, const std::string token, NodeId id, BaseNode* left, BaseNode* right )
		: BaseNode( lineNr, colNr, token, NT_COMPLEX, id )
	{
		m_Left = left;
		m_Right = right;
	}

	void ComplexNode::Release()
	{
		if ( m_Left )
		{
			m_Left->Release();
			delete m_Left;
		}

		if ( m_Right )
		{
			m_Right->Release();
			delete m_Right;
		}
	}

	const BaseNode* ComplexNode::GetLeft() const
	{
		return m_Left;
	}

	const BaseNode* ComplexNode::GetRight() const
	{
		return m_Right;
	}

	void ComplexNode::Compile( Assembler& assembler, uint32_t options )
	{
		auto chunk = assembler.CurrentChunk();
		uint32_t start = ( uint32_t ) chunk->m_Code.size();

		bool discardResult = IS_STATEMENT( options );

		switch ( m_NodeId )
		{
		case NODE_ACCESS_ARRAY:
		{
			m_Left->Compile( assembler, COMPILE_EXPRESSION( COMPILE_NON_ASSIGN( options ) ) );
			m_Right->Compile( assembler, COMPILE_EXPRESSION( COMPILE_NON_ASSIGN( options ) ) );

			if ( IS_ASSIGN_TARGET( options ) )
			{
				// stack@0: value_to_be_assigned
				// stack@1: left_hand_of_access
				EmitByte( QScript::OpCode::OP_SET_PROP_STACK, chunk );

				// Assign node will discard the assignment result if necessary
				discardResult = false;
			}
			else
			{
				// stack@0: left_hand_of_access
				EmitByte( QScript::OpCode::OP_LOAD_PROP_STACK, chunk );
			}
			break;
		}
		case NODE_ACCESS_PROP:
		{
			// Left operand should just produce a value on the stack.
			// Compile a setter
			m_Left->Compile( assembler, COMPILE_EXPRESSION( COMPILE_NON_ASSIGN( options ) ) );

			// Right hand property string
			auto propString = static_cast< ValueNode* >( m_Right )->GetValue();

			if ( IS_ASSIGN_TARGET( options ) )
			{
				// stack@0: value_to_be_assigned
				// stack@1: left_hand_of_access
				EmitConstant( chunk, propString, QScript::OpCode::OP_SET_PROP_SHORT, QScript::OpCode::OP_SET_PROP_LONG, assembler );

				// Assign node will discard the assignment result if necessary
				discardResult = false;
			}
			else
			{
				// stack@0: left_hand_of_access
				EmitConstant( chunk, propString, QScript::OpCode::OP_LOAD_PROP_SHORT, QScript::OpCode::OP_LOAD_PROP_LONG, assembler );
			}

			break;
		}
		case NODE_ASSIGN:
		{
			RequireAssignability( m_Left );

			if ( m_Right->Id() == NODE_FUNC )
			{
				if ( m_Left->Id() == NODE_NAME )
				{
					// Compile a named function
					auto& varName = static_cast< ValueNode* >( m_Left )->GetValue();
					CompileFunction( false, false, false, AS_STRING( varName )->GetString(), static_cast< ListNode* >( m_Right ), assembler );
				}
				else
				{
					// Anonymous function
					CompileFunction( true, false, false, "<anonymous>", static_cast< ListNode* >( m_Right ), assembler );
				}

				m_Left->Compile( assembler, COMPILE_REASSIGN_TARGET( options ) );
			}
			else
			{
				// When assigning a value, first evaluate right hand operand (value)
				// and then set it via the left hand operand
				m_Right->Compile( assembler, COMPILE_EXPRESSION( options ) );
				m_Left->Compile( assembler, COMPILE_REASSIGN_TARGET( options ) );
			}

			auto leftType = m_Left->ExprType( assembler );
			auto rightType = m_Right->ExprType( assembler );

			if ( !( leftType & TYPE_NULL ) && !TypeCheck( leftType, rightType ) )
			{
				throw CompilerException( "cp_invalid_expression_type", "Can not assign expression of type " +
					TypeToString( rightType ) + " to variable of type " + TypeToString( leftType ),
					m_LineNr, m_ColNr, m_Token );
			}
			break;
		}
		case NODE_ASSIGNADD:
		case NODE_ASSIGNDIV:
		case NODE_ASSIGNMOD:
		case NODE_ASSIGNMUL:
		case NODE_ASSIGNSUB:
		{
			// TODO: Faster instruction selection based on number addition or string concatenation
			bool isStringConcat = false;

			RequireAssignability( m_Left );

			// Load both values to the top of the stack
			m_Left->Compile( assembler, COMPILE_EXPRESSION( options ) );
			m_Right->Compile( assembler, COMPILE_EXPRESSION( options ) );

			// Perform type checking
			if ( m_NodeId == NODE_ASSIGNADD )
			{
				// Either add or concat (strings)
				auto leftType = m_Left->ExprType( assembler );
				auto rightType = m_Right->ExprType( assembler );

				auto leftString = !!( leftType & TYPE_STRING );
				auto rightString = !!( rightType & TYPE_STRING );

				if ( !TypeCheck( leftType, Type_t( TYPE_NUMBER ) ) && !leftString )
				{
					throw CompilerException( "cp_invalid_expression_type", "Expected variable of type " +
						TypeToString( Type_t( TYPE_NUMBER | TYPE_STRING ) ) + ", got: " + TypeToString( leftType ),
						m_Left->LineNr(), m_Left->ColNr(), m_Left->Token() );
				}

				if ( !TypeCheck( rightType, Type_t( TYPE_NUMBER ) ) && !rightString )
				{
					throw CompilerException( "cp_invalid_expression_type", "Expected expression of type " +
						TypeToString( Type_t( TYPE_NUMBER | TYPE_STRING ) ) + ", got: " + TypeToString( rightType ),
						m_Right->LineNr(), m_Right->ColNr(), m_Right->Token() );
				}

				// If the variable is a num, but right hand operand is a string, this would
				// lead to an unexpected type conversion on runtime.
				if ( ( leftType & Type_t( TYPE_NUMBER ) ) && rightString )
				{
					throw CompilerException( "cp_invalid_expression_type", "Expected variable of type " +
						TypeToString( Type_t( TYPE_STRING ) ) + ", got: " + TypeToString( leftType ),
						m_Left->LineNr(), m_Left->ColNr(), m_Left->Token() );
				}

				isStringConcat = leftString;
			}
			else
			{
				// Both types must be numbers
				auto leftType = m_Left->ExprType( assembler );
				auto rightType = m_Right->ExprType( assembler );

				if ( !TypeCheck( leftType, Type_t( TYPE_NUMBER ) ) )
				{
					throw CompilerException( "cp_invalid_expression_type", "Can not assign expression of type " +
						TypeToString( Type_t( TYPE_NUMBER ) ) + " to variable of type " + TypeToString( leftType ),
						m_Left->LineNr(), m_Left->ColNr(), m_Left->Token() );
				}

				if ( !TypeCheck( rightType, Type_t( TYPE_NUMBER ) ) )
				{
					throw CompilerException( "cp_invalid_expression_type", "Expected expression of type " +
						TypeToString( Type_t( TYPE_NUMBER ) ) + ", got: " + TypeToString( rightType ),
						m_Right->LineNr(), m_Right->ColNr(), m_Right->Token() );
				}
			}

			// Add respective nullary operator
			std::map< NodeId, QScript::OpCode > map = {
				{ NODE_ASSIGNADD, 	QScript::OpCode::OP_ADD },
				{ NODE_ASSIGNDIV, 	QScript::OpCode::OP_DIV },
				{ NODE_ASSIGNSUB, 	QScript::OpCode::OP_SUB },
				{ NODE_ASSIGNMUL, 	QScript::OpCode::OP_MUL },
				{ NODE_ASSIGNMOD, 	QScript::OpCode::OP_MOD },
			};

			EmitByte( map[ m_NodeId ], chunk );

			// Compile setter
			m_Left->Compile( assembler, COMPILE_REASSIGN_TARGET( options ) );
			break;
		}
		case NODE_AND:
		{
			m_Left->Compile( assembler, COMPILE_EXPRESSION( options ) );

			uint32_t endJump = ( uint32_t ) chunk->m_Code.size();

			// Left hand operand evaluated to true, discard it and return the right hand result
			EmitByte( QScript::OpCode::OP_POP, chunk );

			m_Right->Compile( assembler, COMPILE_EXPRESSION( options ) );

			// Create jump instruction
			PlaceJump( chunk, endJump, ( uint32_t ) chunk->m_Code.size() - endJump, QScript::OpCode::OP_JUMP_IF_ZERO_SHORT, QScript::OpCode::OP_JUMP_IF_ZERO_LONG );
			break;
		}
		case NODE_CALL:
		{
			auto args = static_cast< ListNode* >( m_Right )->GetList();

			if ( args.size() > 255 )
				throw CompilerException( "cp_too_many_args", "Too many arguments for a function call", m_LineNr, m_ColNr, m_Token );

			// Function object
			m_Left->Compile( assembler, COMPILE_EXPRESSION( options ) );

			// Check arity if target can be resolved
			if ( m_Left->Id() == NODE_NAME )
			{
				auto varNameValue = static_cast< ValueNode* >( m_Left )->GetValue();

				if ( IS_STRING( varNameValue ) )
				{
					auto varName = AS_STRING( varNameValue )->GetString();

					auto findFunction = [ &assembler ]( const std::string& name ) -> QScript::FunctionObject*
					{
						Variable_t varInfo;
						uint32_t nameIndex;

						if ( !assembler.FindArgument( name, &varInfo ) &&
							!assembler.FindLocal( name, &nameIndex, &varInfo ) &&
							!assembler.FindUpvalue( name, &nameIndex, &varInfo ) &&
							!assembler.FindGlobal( name, &varInfo ) )
						{
							return NULL;
						}

						if ( !varInfo.m_IsConst )
							return NULL;

						if ( varInfo.m_Type.m_Bits != TYPE_FUNCTION )
							return NULL;

						return varInfo.m_Function;
					};

					auto function = findFunction( varName );

					if ( function )
					{
						int lineNr = LineNr();
						int colNr = ColNr();
						std::string token = Token();

						if ( args.size() > 0 )
						{
							auto lastArg = args[ args.size() - 1 ];

							lineNr = lastArg->LineNr();
							colNr = lastArg->ColNr();
							token = lastArg->Token();
						}

						if ( function->NumArgs() > ( int ) args.size() )
						{
							throw CompilerException( "cp_invalid_call_arity", "Calling function \""
								+ function->GetName() + "\" with too few arguments. Function accepts "
								+ std::to_string( function->NumArgs() ) + " args, got "
								+ std::to_string( args.size() ), lineNr, colNr, token );
						}
						else if ( function->NumArgs() < ( int ) args.size() )
						{
							throw CompilerException( "cp_invalid_call_arity", "Calling function \""
								+ function->GetName() + "\" with too many arguments. Function accepts "
								+ std::to_string( function->NumArgs() ) + " args, got "
								+ std::to_string( args.size() ), lineNr, colNr, token );
						}
					}
				}
			}

			// Push arguments to stack
			for ( auto arg : args )
				arg->Compile( assembler, COMPILE_EXPRESSION( options ) );

			if ( args.size() < QScript::OP_CALL_MAX )
			{
				EmitByte( QScript::OP_CALL_0 + ( uint8_t ) args.size(), chunk );
			}
			else
			{
				EmitByte( QScript::OpCode::OP_CALL, chunk );
				EmitByte( ( uint8_t ) args.size(), chunk );
			}

			break;
		}
		case NODE_OR:
		{
			m_Left->Compile( assembler, COMPILE_EXPRESSION( options ) );

			uint32_t endJump = ( uint32_t ) chunk->m_Code.size();

			// Pop left operand off
			EmitByte( QScript::OpCode::OP_POP, chunk );

			m_Right->Compile( assembler, COMPILE_EXPRESSION( options ) );

			uint32_t patchSize = PlaceJump( chunk, endJump, ( uint32_t ) chunk->m_Code.size() - endJump, QScript::OpCode::OP_JUMP_SHORT, QScript::OpCode::OP_JUMP_LONG );

			PlaceJump( chunk, endJump, patchSize, QScript::OpCode::OP_JUMP_IF_ZERO_SHORT, QScript::OpCode::OP_JUMP_IF_ZERO_LONG );
			break;
		}
		case NODE_DEC:
		case NODE_INC:
		{
			bool isPost = ( m_Left != NULL ) && !IS_STATEMENT( options );
			auto node = ( m_Left ? m_Left : m_Right );

			RequireAssignability( node );

			// Perform type checking
			auto targetType = node->ExprType( assembler );
			if ( !TypeCheck( targetType, Type_t( TYPE_NUMBER ) ) )
			{
				throw CompilerException( "cp_invalid_expression_type", "Can not assign expression of type " +
					TypeToString( Type_t( TYPE_NUMBER ) ) + " to variable of type " + TypeToString( targetType ),
					m_LineNr, m_ColNr, m_Token );
			}

			// Place target variable's value on the stack
			node->Compile( assembler, COMPILE_EXPRESSION( options ) );

			if ( isPost )
			{
				// Once again, place the current value on top of the stack.
				// We'll use the previously pushed value as the result of this
				// expression (original value of the variable)
				node->Compile( assembler, COMPILE_EXPRESSION( options ) );
			}

			// Load number 1 & add/sub
			EmitByte( QScript::OP_LOAD_1, chunk );
			EmitByte( m_NodeId == NODE_INC ? QScript::OP_ADD : QScript::OP_SUB, chunk );

			// Assign to target (compile setter)
			node->Compile( assembler, COMPILE_REASSIGN_TARGET( options ) );

			// If it is post inc/dec, pop updated value off stack
			if ( isPost )
				EmitByte( QScript::OP_POP, chunk );

			break;
		}
		default:
		{
			m_Left->Compile( assembler, COMPILE_EXPRESSION( options ) );
			m_Right->Compile( assembler, COMPILE_EXPRESSION( options ) );

			// Note: All the opcodes listed here MUST use both left and right hand values
			// and produce a new value on the stack (stack effect = -1)
			std::map< NodeId, QScript::OpCode > numberOps = {
				{ NODE_SUB, 			QScript::OpCode::OP_SUB },
				{ NODE_MUL, 			QScript::OpCode::OP_MUL },
				{ NODE_DIV, 			QScript::OpCode::OP_DIV },
				{ NODE_MOD, 			QScript::OpCode::OP_MOD },
				{ NODE_POW, 			QScript::OpCode::OP_POW },
				{ NODE_GREATERTHAN,		QScript::OpCode::OP_GREATERTHAN },
				{ NODE_GREATEREQUAL,	QScript::OpCode::OP_GREATERTHAN_OR_EQUAL },
				{ NODE_LESSTHAN,		QScript::OpCode::OP_LESSTHAN },
				{ NODE_LESSEQUAL,		QScript::OpCode::OP_LESSTHAN_OR_EQUAL },
			};

			std::map< NodeId, QScript::OpCode > otherOps ={
				{ NODE_ADD, 			QScript::OpCode::OP_ADD },
				{ NODE_EQUALS,			QScript::OpCode::OP_EQUALS },
				{ NODE_NOTEQUALS,		QScript::OpCode::OP_NOT_EQUALS },
			};

			auto leftType = m_Left->ExprType( assembler );
			auto rightType = m_Right->ExprType( assembler );

			auto opCode = numberOps.find( m_NodeId );
			if ( opCode != numberOps.end() )
			{
				// Type check: Number
				if ( !TypeCheck( leftType, Type_t( TYPE_NUMBER ) ) )
				{
					throw CompilerException( "cp_invalid_expression_type", "Expected expression of type " +
						TypeToString( Type_t( TYPE_NUMBER ) ) + ", got: " + TypeToString( leftType ),
						m_Left->LineNr(), m_Left->ColNr(), m_Left->Token() );
				}

				if ( !TypeCheck( rightType, Type_t( TYPE_NUMBER ) ) )
				{
					throw CompilerException( "cp_invalid_expression_type", "Expected expression of type " +
						TypeToString( Type_t( TYPE_NUMBER ) ) + ", got: " + TypeToString( rightType ),
						m_Right->LineNr(), m_Right->ColNr(), m_Right->Token() );
				}

				EmitByte( opCode->second, chunk );
			}
			else if ( ( opCode = otherOps.find( m_NodeId ) ) != otherOps.end() )
			{
				// TODO: instruction selection
				bool isStringConcat = false;

				if ( m_NodeId == NODE_ADD )
				{
					// Type check: String or number
					auto leftString = !!( leftType & TYPE_STRING );
					auto rightString = !!( rightType & TYPE_STRING );
					// left = NULL + number
					if ( !TypeCheck( leftType, Type_t( TYPE_NUMBER ) ) && !leftString )
					{
						throw CompilerException( "cp_invalid_expression_type", "Expected expression of type " +
							TypeToString( Type_t( TYPE_NUMBER | TYPE_STRING ) ) + ", got: " + TypeToString( leftType ),
							m_Left->LineNr(), m_Left->ColNr(), m_Left->Token() );
					}

					if ( !TypeCheck( rightType, Type_t( TYPE_NUMBER ) ) && !rightString )
					{
						throw CompilerException( "cp_invalid_expression_type", "Expected expression of type " +
							TypeToString( Type_t( TYPE_NUMBER | TYPE_STRING ) ) + ", got: " + TypeToString( rightType ),
							m_Right->LineNr(), m_Right->ColNr(), m_Right->Token() );
					}

					isStringConcat = rightString || leftString;
				}

				EmitByte( opCode->second, chunk );
			}
			else
			{
				throw CompilerException( "cp_invalid_complex_node", "Unknown complex node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
			}

			break;
		}
		}

		if ( discardResult )
			EmitByte( QScript::OpCode::OP_POP, chunk );

		AddDebugSymbol( assembler, start, m_LineNr, m_ColNr, m_Token );
	}

	SimpleNode::SimpleNode( int lineNr, int colNr, const std::string token, NodeId id, BaseNode* node )
		: BaseNode( lineNr, colNr, token, NT_SIMPLE, id )
	{
		m_Node = node;
	}

	void SimpleNode::Release()
	{
		if ( m_Node )
		{
			m_Node->Release();
			delete m_Node;
		}
	}

	const BaseNode* SimpleNode::GetNode() const
	{
		return m_Node;
	}

	void SimpleNode::Compile( Assembler& assembler, uint32_t options )
	{
		auto chunk = assembler.CurrentChunk();
		uint32_t start = ( uint32_t ) chunk->m_Code.size();

		if ( m_NodeId == NODE_IMPORT )
		{
			if ( !assembler.IsTopLevel() || assembler.StackDepth() > 0 )
			{
				throw CompilerException( "cp_non_top_level_import", "Imports must be declared at top-level only",
					LineNr(), ColNr(), m_Token );
			}

			auto moduleName = static_cast< ValueNode* >( m_Node )->GetValue();
			auto module = QScript::ResolveModule( AS_STRING( moduleName )->GetString() );

			if ( module == NULL )
			{
				throw CompilerException( "cp_unknown_module",
					"Unknown module: \"" + AS_STRING( moduleName )->GetString() + "\"",
					m_Node->LineNr(), m_Node->ColNr(), m_Node->Token() );
			}

			// Import into compiler context
			module->Import( &assembler, m_Node->LineNr(), m_Node->ColNr() );

			// Import in VM
			uint32_t constIndex = AddConstant( moduleName, chunk );
			EmitByte( QScript::OpCode::OP_IMPORT, chunk );
			EmitByte( ENCODE_LONG( constIndex, 0 ), chunk );
			EmitByte( ENCODE_LONG( constIndex, 1 ), chunk );
			EmitByte( ENCODE_LONG( constIndex, 2 ), chunk );
			EmitByte( ENCODE_LONG( constIndex, 3 ), chunk );
		}
		else
		{
			std::map< NodeId, QScript::OpCode > singleByte ={
				{ NODE_RETURN, 			QScript::OpCode::OP_RETURN },
				{ NODE_NOT, 			QScript::OpCode::OP_NOT },
				{ NODE_NEG, 			QScript::OpCode::OP_NEGATE },
			};

			if ( m_NodeId == NODE_RETURN )
			{
				// Check return type match
				auto type = assembler.CurrentContext()->m_Type;
				auto retnType = type.m_ReturnType ? *type.m_ReturnType : Type_t( TYPE_UNKNOWN );
				auto exprType = m_Node ? m_Node->ExprType( assembler ) : Type_t( TYPE_NULL );

				if ( retnType.IsUnknown() && exprType.IsUnknown() )
				{
					if ( !( retnType & exprType ) )
					{
						throw CompilerException( "cp_invalid_expression_type", "Expected expression of type " +
							TypeToString( retnType ) + ", got: " + TypeToString( exprType ),
							LineNr(), ColNr(), Token() );
					}
				}
			}

			auto opCode = singleByte.find( m_NodeId );
			if ( opCode != singleByte.end() )
			{
				if ( m_Node )
					m_Node->Compile( assembler, COMPILE_EXPRESSION( options ) );
				else if ( m_NodeId == NODE_RETURN )
					EmitByte( QScript::OpCode::OP_LOAD_NULL, chunk );

				EmitByte( opCode->second, chunk );
			}
			else
			{
				throw CompilerException( "cp_invalid_simple_node", "Unknown simple node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
			}
		}

		AddDebugSymbol( assembler, start, m_LineNr, m_ColNr, m_Token );
	}

	ListNode::ListNode( int lineNr, int colNr, const std::string token, NodeId id, const std::vector< BaseNode* >& nodeList )
		: BaseNode( lineNr, colNr, token, NT_LIST, id )
	{
		m_NodeList = nodeList;
	}

	void ListNode::Release()
	{
		for ( auto node : m_NodeList )
		{
			if ( node )
			{
				node->Release();
				delete node;
			}
		}

		m_NodeList.clear();
	}

	const std::vector< BaseNode* >& ListNode::GetList() const
	{
		return m_NodeList;
	}

	void ListNode::Compile( Assembler& assembler, uint32_t options )
	{
		auto chunk = assembler.CurrentChunk();
		uint32_t start = ( uint32_t ) chunk->m_Code.size();

		switch ( m_NodeId )
		{
		case NODE_TABLE:
		{
			auto isStatement = IS_STATEMENT( options );
			bool isLocal = ( assembler.StackDepth() > 0 );

			QScript::Value varName;
			std::string varNameString = "";

			if ( m_NodeList[ 0 ] )
			{
				varName = static_cast< ValueNode* >( m_NodeList[ 0 ] )->GetValue();
				varNameString = AS_STRING( varName )->GetString();
			}

			int lineNr = m_NodeList[ 0 ] ? m_NodeList[ 0 ]->LineNr() : -1;
			int colNr = m_NodeList[ 0 ] ? m_NodeList[ 0 ]->ColNr() : -1;

			// Empty table
			EmitConstant( chunk, MAKE_STRING( varNameString ), QScript::OpCode::OP_CREATE_TABLE_SHORT, QScript::OpCode::OP_CREATE_TABLE_LONG, assembler );

			if ( isStatement )
			{
				if ( !m_NodeList[ 0 ] )
				{
					throw CompilerException( "ir_table_name", "Expected a named table when declaring a statement",
						LineNr(), ColNr(), Token() );
				}

				if ( !isLocal )
				{
					// Global variable
					if ( !assembler.AddGlobal( varNameString, true, lineNr, colNr, TYPE_TABLE ) )
					{
						throw CompilerException( "cp_identifier_already_exists", "Identifier \"" + varNameString + "\" already exists",
							m_LineNr, m_ColNr, m_Token );
					}

					EmitConstant( chunk, varName, QScript::OpCode::OP_SET_GLOBAL_SHORT, QScript::OpCode::OP_SET_GLOBAL_LONG, assembler );
				}
				else
				{
					// Local variable
					assembler.AddLocal( varNameString, true, lineNr, colNr, TYPE_TABLE );
				}
			}
			else
			{
				// Not a statement? We are creating a nested table
			}

			// Populate table with properties?
			if ( m_NodeList[ 1 ] )
			{
				auto propertyList = static_cast< ListNode* >( m_NodeList[ 1 ] );

				for ( auto prop : propertyList->GetList() )
				{
					QScript::Value propName;

					if ( prop->Id() == NODE_FIELD )
					{
						auto propNode = static_cast< ListNode* >( prop );
						auto propNameNode = propNode->GetList()[ 0 ];
						// auto propTypeNode = propNode->GetList()[ 1 ]; // TODO: Compile time table templates for type support
						auto defaultValueNode = propNode->GetList()[ 2 ];
						propName = static_cast< ValueNode* >( propNameNode )->GetValue();

						if ( defaultValueNode )
						{
							defaultValueNode->Compile( assembler, COMPILE_EXPRESSION( options ) );
						}
						else
						{
							// Load null (no default value given)
							EmitByte( QScript::OpCode::OP_LOAD_NULL, chunk );
						}
					}
					else if ( prop->Id() == NODE_METHOD )
					{
						auto propNode = static_cast< ListNode* >( prop );
						auto funcNode = static_cast< ListNode* >( propNode->GetList()[ 1 ] );
						auto propNameNode = propNode->GetList()[ 0 ];

						propName = static_cast< ValueNode* >( propNameNode )->GetValue();
						CompileFunction( true, true, true, varNameString + "::" + AS_STRING( propName )->GetString(), funcNode, assembler, NULL,
							propNameNode->LineNr(), propNameNode->ColNr() );

						EmitByte( QScript::OpCode::OP_LOAD_TOP_SHORT, chunk );
						EmitByte( 1, chunk );

						EmitByte( QScript::OpCode::OP_LOAD_TOP_SHORT, chunk );
						EmitByte( 1, chunk );

						EmitByte( QScript::OpCode::OP_BIND, chunk );
					}
					else if ( prop->Id() == NODE_TABLE )
					{
						// Nested table
						auto subTable = static_cast< ListNode* >( prop );
						subTable->Compile( assembler, COMPILE_EXPRESSION( options ) );

						auto propNameNode = subTable->GetList()[ 0 ];
						propName = static_cast< ValueNode* >( propNameNode )->GetValue();
					}
					else if ( prop->Id() == NODE_ARRAY )
					{
						auto subArray = static_cast< ListNode* >( prop );
						subArray->Compile( assembler, COMPILE_EXPRESSION( options ) );

						auto propNameNode = subArray->GetList()[ 0 ];

						if ( propNameNode == NULL )
						{
							throw CompilerException( "ir_array_name", "Expected a named array inside table",
								subArray->LineNr(), subArray->ColNr(), subArray->Token() );
						}

						propName = static_cast< ValueNode* >( propNameNode )->GetValue();
					}
					else
					{
						throw CompilerException( "cp_invalid_table_child", "Unknown child node: \"" + prop->Token() + "\"",
							prop->LineNr(), prop->ColNr(), prop->Token() );
					}

					/*
						At this point our stack looks something like this
							stack@0: <some local>
							stack@1: <some local>
							stack@2: <table object>
							stack@3: <default value>

						Duplicate table object and set field to default value
					*/
					EmitByte( QScript::OpCode::OP_LOAD_TOP_SHORT, chunk );
					EmitByte( 1, chunk );

					// Set prop
					EmitConstant( chunk, propName, QScript::OpCode::OP_SET_PROP_SHORT, QScript::OpCode::OP_SET_PROP_LONG, assembler );

					// Pop default value off
					EmitByte( QScript::OpCode::OP_POP, chunk );
				}
			}

			// Not a local? pop it off the stack
			if ( !isLocal && isStatement )
				EmitByte( QScript::OpCode::OP_POP, chunk );
			break;
		}
		case NODE_ARRAY:
		{
			auto isStatement = IS_STATEMENT( options );
			bool isLocal = ( assembler.StackDepth() > 0 );

			QScript::Value varName;
			std::string varNameString = "";

			if ( m_NodeList[ 0 ] )
			{
				varName = static_cast< ValueNode* >( m_NodeList[ 0 ] )->GetValue();
				varNameString = AS_STRING( varName )->GetString();
			}

			int lineNr = m_NodeList[ 0 ] ? m_NodeList[ 0 ]->LineNr() : -1;
			int colNr = m_NodeList[ 0 ] ? m_NodeList[ 0 ]->ColNr() : -1;

			// Empty array
			EmitConstant( chunk, MAKE_STRING( varNameString ), QScript::OpCode::OP_CREATE_ARRAY_SHORT, QScript::OpCode::OP_CREATE_ARRAY_LONG, assembler );

			if ( isStatement )
			{
				if ( !m_NodeList[ 0 ] )
				{
					throw CompilerException( "ir_array_name", "Expected a named array when declaring a statement",
						LineNr(), ColNr(), Token() );
				}

				if ( !isLocal )
				{
					if ( !assembler.AddGlobal( varNameString, true, lineNr, colNr, TYPE_ARRAY ) )
					{
						throw CompilerException( "cp_identifier_already_exists", "Identifier \"" + varNameString + "\" already exists",
							m_LineNr, m_ColNr, m_Token );
					}

					EmitConstant( chunk, varName, QScript::OpCode::OP_SET_GLOBAL_SHORT, QScript::OpCode::OP_SET_GLOBAL_LONG, assembler );
				}
				else
				{
					assembler.AddLocal( varNameString, true, lineNr, colNr, TYPE_ARRAY );
				}
			}

			// Initializer list?
			if ( m_NodeList[ 1 ] )
			{
				auto propertyList = static_cast< ListNode* >( m_NodeList[ 1 ] );

				for ( auto prop : propertyList->GetList() )
				{
					prop->Compile( assembler, COMPILE_EXPRESSION( options ) );
					EmitByte( QScript::OpCode::OP_PUSH_ARRAY, chunk );
				}
			}

			// Not a local? pop it off the stack
			if ( !isLocal && isStatement )
				EmitByte( QScript::OpCode::OP_POP, chunk );
			break;
		}
		case NODE_DO:
		{
			if ( !IS_STATEMENT( options ) )
				throw EXPECTED_EXPRESSION;

			// Jump over wrapper to pop conditional value
			uint32_t condWrapperJump = ( uint32_t ) chunk->m_Code.size();

			// Pop condition value
			EmitByte( QScript::OpCode::OP_POP, chunk );

			// Jump over wrapper on first iteration
			uint32_t condWrapperBegin = condWrapperJump;
			condWrapperBegin += PlaceJump( chunk, condWrapperJump, ( uint32_t ) chunk->m_Code.size() - condWrapperJump,
				QScript::OpCode::OP_JUMP_SHORT, QScript::OpCode::OP_JUMP_LONG );

			// Address to loop back to
			uint32_t bodyBegin = ( uint32_t ) chunk->m_Code.size();

			// Loop body
			m_NodeList[ 0 ]->Compile( assembler, COMPILE_STATEMENT( options ) );

			// Loop condition
			m_NodeList[ 1 ]->Compile( assembler, COMPILE_EXPRESSION( options ) );

			uint32_t backJumpAddress = ( uint32_t ) chunk->m_Code.size();
			uint32_t backJumpSize = backJumpAddress - condWrapperBegin;

			// Jump back to top
			uint32_t backJumpPatchSize = PlaceJump( chunk, ( uint32_t ) chunk->m_Code.size(), backJumpSize + 5,
				QScript::OpCode::OP_JUMP_BACK_SHORT, QScript::OpCode::OP_JUMP_BACK_LONG );

			// Skipping jump if condition is false
			uint32_t overJumpPatchSize = PlaceJump( chunk, ( uint32_t ) chunk->m_Code.size() - backJumpPatchSize, backJumpPatchSize,
				QScript::OpCode::OP_JUMP_IF_ZERO_SHORT, QScript::OpCode::OP_JUMP_IF_ZERO_LONG );

			// Correct backjump offset
			PatchJump( chunk, backJumpAddress + overJumpPatchSize, backJumpSize + overJumpPatchSize,
				QScript::OpCode::OP_JUMP_BACK_SHORT, QScript::OpCode::OP_JUMP_BACK_LONG );

			// Pop condition value
			EmitByte( QScript::OpCode::OP_POP, chunk );
			break;
		}
		case NODE_FOR:
		{
			if ( !IS_STATEMENT( options ) )
				throw EXPECTED_EXPRESSION;

			assembler.PushScope();

			// Compile initializer
			if ( m_NodeList[ 0 ] )
				m_NodeList[ 0 ]->Compile( assembler, COMPILE_STATEMENT( options ) );

			uint32_t loopConditionBegin = ( uint32_t ) chunk->m_Code.size();

			// Compile condition
			if ( m_NodeList[ 1 ] )
				m_NodeList[ 1 ]->Compile( assembler, COMPILE_EXPRESSION( options ) );

			// Jump to loop end
			uint32_t loopSkipJump = ( uint32_t ) chunk->m_Code.size();

			// Pop condition value
			EmitByte( QScript::OpCode::OP_POP, chunk );

			uint32_t loopIncrementBegin = ( uint32_t ) chunk->m_Code.size();

			// Increment statement
			if ( m_NodeList[ 2 ] )
				m_NodeList[ 2 ]->Compile( assembler, COMPILE_STATEMENT( options ) );

			// Jump to condition (must be patched)
			uint32_t loopConditionJump = ( uint32_t ) chunk->m_Code.size();
			uint32_t jumpToConditionSize = PlaceJump( chunk, loopConditionJump, loopConditionJump - loopConditionBegin + 5,
				QScript::OpCode::OP_JUMP_BACK_SHORT, QScript::OpCode::OP_JUMP_BACK_LONG );

			// Compile loop body
			if ( m_NodeList[ 3 ] )
				m_NodeList[ 3 ]->Compile( assembler, COMPILE_STATEMENT( options ) );

			// Jump to increment statement
			PlaceJump( chunk, ( uint32_t ) chunk->m_Code.size(), ( uint32_t ) chunk->m_Code.size() - loopIncrementBegin,
				QScript::OpCode::OP_JUMP_BACK_SHORT, QScript::OpCode::OP_JUMP_BACK_LONG );

			// Jump over increment on first fall through
			uint32_t jumpToBodyOnFirstSize = PlaceJump( chunk, loopIncrementBegin, ( loopConditionJump - loopIncrementBegin ) + jumpToConditionSize,
				QScript::OpCode::OP_JUMP_SHORT, QScript::OpCode::OP_JUMP_LONG );

			// Jump to loop end
			uint32_t jumpToLoopEndSize = PlaceJump( chunk, loopSkipJump, ( uint32_t ) chunk->m_Code.size() - loopSkipJump,
				QScript::OpCode::OP_JUMP_IF_ZERO_SHORT, QScript::OpCode::OP_JUMP_IF_ZERO_LONG );

			// Pop condition value
			EmitByte( QScript::OpCode::OP_POP, chunk );

			PatchJump( chunk, loopConditionJump + jumpToLoopEndSize + jumpToBodyOnFirstSize,
				( loopConditionJump + jumpToBodyOnFirstSize + jumpToLoopEndSize ) - loopConditionBegin,
				QScript::OpCode::OP_JUMP_BACK_SHORT, QScript::OpCode::OP_JUMP_BACK_LONG );

			assembler.PopScope();
			break;
		}
		case NODE_FUNC:
		{
			if ( IS_STATEMENT( options ) )
				throw EXPECTED_STATEMENT;

			CompileFunction( true, false, false, "<anonymous>", this, assembler );
			break;
		}
		case NODE_INLINE_IF:
		case NODE_IF:
		{
			bool isInline = ( m_NodeId == NODE_INLINE_IF );

			if ( isInline )
			{
				if ( IS_STATEMENT( options ) )
					throw EXPECTED_STATEMENT;
			}
			else
			{
				if ( !IS_STATEMENT( options ) )
					throw EXPECTED_EXPRESSION;
			}

			// Compile condition, now the result is at the top of the stack
			m_NodeList[ 0 ]->Compile( assembler, COMPILE_EXPRESSION( options ) );

			// Address of the next instruction from the jump
			uint32_t thenBodyBegin = ( uint32_t ) chunk->m_Code.size();

			// Pop condition value off stack
			EmitByte( QScript::OpCode::OP_POP, chunk );

			// Compile body
			m_NodeList[ 1 ]->Compile( assembler, isInline ? COMPILE_EXPRESSION( options ) : COMPILE_STATEMENT( options ) );

			uint32_t elseBodyBegin = ( uint32_t ) chunk->m_Code.size();

			// Pop condition value off stack
			EmitByte( QScript::OpCode::OP_POP, chunk );

			// Compile optional else-branch
			if ( m_NodeList[ 2 ] )
				m_NodeList[ 2 ]->Compile( assembler, isInline ? COMPILE_EXPRESSION( options ) : COMPILE_STATEMENT( options ) );

			uint32_t elseBodyEnd = ( uint32_t ) chunk->m_Code.size();

			// Jump over else branch
			elseBodyBegin += PlaceJump( chunk, elseBodyBegin, elseBodyEnd - elseBodyBegin,
				QScript::OpCode::OP_JUMP_SHORT, QScript::OpCode::OP_JUMP_LONG );

			// Create jump instruction
			PlaceJump( chunk, thenBodyBegin, elseBodyBegin - thenBodyBegin,
				QScript::OpCode::OP_JUMP_IF_ZERO_SHORT, QScript::OpCode::OP_JUMP_IF_ZERO_LONG );

			break;
		}
		case NODE_SCOPE:
		{
			if ( !IS_STATEMENT( options ) )
				throw EXPECTED_EXPRESSION;

			assembler.PushScope();

			// Compile each statement in scope body
			for ( auto node : m_NodeList )
				node->Compile( assembler, COMPILE_STATEMENT( options ) );

			assembler.PopScope();
			break;
		}
		case NODE_CONSTVAR:
		case NODE_VAR:
		{
			// m_NodeList[ 0 ] should be NODE_NAME and is validated during parsing
			auto& varName = static_cast< ValueNode* >( m_NodeList[ 0 ] )->GetValue();
			auto& varTypeValue = static_cast< ValueNode* >( m_NodeList[ 2 ] )->GetValue();

			auto varString = AS_STRING( varName )->GetString();
			auto varType = Type_t( ( uint32_t ) AS_NUMBER( varTypeValue ) );
			uint32_t varReturnType = TYPE_UNKNOWN;

			bool isLocal = ( assembler.StackDepth() > 0 );
			bool isConst = ( m_NodeId == NODE_CONSTVAR );
			QScript::FunctionObject* fn = NULL;

			int lineNr = m_NodeList[ 0 ]->LineNr();
			int colNr = m_NodeList[ 0 ]->ColNr();

			// Assign now?
			if ( m_NodeList[ 1 ] )
			{
				auto exprType = m_NodeList[ 1 ]->ExprType( assembler );

				if ( !varType.IsUnknown() && !( varType & TYPE_AUTO ) )
				{
					if ( !TypeCheck( varType, exprType ) )
					{
						throw CompilerException( "cp_invalid_expression_type", "Can not assign expression of type " +
							TypeToString( exprType ) + " to variable of type " + TypeToString( varType ),
							m_NodeList[ 1 ]->LineNr(), m_NodeList[ 1 ]->ColNr(), m_NodeList[ 1 ]->Token() );
					}
				}
				else if ( varType & TYPE_AUTO )
				{
					// Deduce type from expression
					varType = exprType;
				}

				if ( m_NodeList[ 1 ]->Id() == NODE_FUNC )
				{
					// Compile a named function
					fn = CompileFunction( false, isConst, false, varString, static_cast< ListNode* >( m_NodeList[ 1 ] ), assembler, &varReturnType, lineNr, colNr );
					varType = TYPE_FUNCTION;
				}
				else
				{
					// Assign variable at declaration
					m_NodeList[ 1 ]->Compile( assembler, COMPILE_EXPRESSION( options ) );
				}

				if ( isLocal )
				{
					assembler.AddLocal( varString, isConst, lineNr, colNr, varType, varReturnType, fn );
				}
				else
				{
					if ( !assembler.AddGlobal( varString, isConst, lineNr, colNr, varType, varReturnType, fn ) )
					{
						throw CompilerException( "cp_identifier_already_exists", "Identifier \"" + varString + "\" already exists",
							lineNr, colNr, m_NodeList[ 0 ]->Token() );
					}

					m_NodeList[ 0 ]->Compile( assembler, COMPILE_ASSIGN_TARGET( options ) );

					if ( IS_STATEMENT( options ) )
						EmitByte( QScript::OpCode::OP_POP, chunk );
				}
			}
			else
			{
				if ( !IS_STATEMENT( options ) )
					throw EXPECTED_EXPRESSION;

				// Empty variable
				EmitByte( QScript::OpCode::OP_LOAD_NULL, chunk );

				if ( !TypeCheck( varType, TYPE_NULL ) )
				{
					throw CompilerException( "cp_invalid_expression_type", "Expected expression of type " +
						TypeToString( varType ) + ", got: " + TypeToString( TYPE_NULL ),
						lineNr, colNr, m_NodeList[ 0 ]->Token() );
				}

				if ( !isLocal )
				{
					// Global variable
					if ( !assembler.AddGlobal( varString, isConst, lineNr, colNr, TYPE_UNKNOWN ) )
					{
						throw CompilerException( "cp_identifier_already_exists", "Identifier \"" + varString + "\" already exists",
							m_NodeList[ 0 ]->LineNr(), m_NodeList[ 0 ]->ColNr(), m_NodeList[ 0 ]->Token() );
					}

					EmitConstant( chunk, varName, QScript::OpCode::OP_SET_GLOBAL_SHORT, QScript::OpCode::OP_SET_GLOBAL_LONG, assembler );
					EmitByte( QScript::OpCode::OP_POP, chunk );
				}
				else
				{
					// Local variable
					assembler.AddLocal( varString, isConst, lineNr, colNr, TYPE_UNKNOWN );
				}
			}
			break;
		}
		case NODE_WHILE:
		{
			if ( !IS_STATEMENT( options ) )
				throw EXPECTED_EXPRESSION;

			uint32_t loopConditionBegin = ( uint32_t ) chunk->m_Code.size();

			// Compile condition
			m_NodeList[ 0 ]->Compile( assembler, COMPILE_EXPRESSION( options ) );

			uint32_t loopBodyBegin = ( uint32_t ) chunk->m_Code.size();

			EmitByte( QScript::OpCode::OP_POP, chunk );

			// Compile body
			m_NodeList[ 1 ]->Compile( assembler, COMPILE_STATEMENT( options ) );

			uint32_t firstJumpSize = ( uint32_t ) chunk->m_Code.size() - loopBodyBegin;
			PlaceJump( chunk, loopBodyBegin, firstJumpSize + 5,
				QScript::OpCode::OP_JUMP_IF_ZERO_SHORT, QScript::OpCode::OP_JUMP_IF_ZERO_LONG );

			uint32_t patchSize = PlaceJump( chunk, ( uint32_t ) chunk->m_Code.size(), ( uint32_t ) chunk->m_Code.size() - loopConditionBegin,
				QScript::OpCode::OP_JUMP_BACK_SHORT, QScript::OpCode::OP_JUMP_BACK_LONG );

			PatchJump( chunk, loopBodyBegin, firstJumpSize + patchSize,
				QScript::OpCode::OP_JUMP_IF_ZERO_SHORT, QScript::OpCode::OP_JUMP_IF_ZERO_LONG );

			EmitByte( QScript::OpCode::OP_POP, chunk );
			break;
		}
		default:
			throw CompilerException( "cp_invalid_list_node", "Unknown list node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}

		AddDebugSymbol( assembler, start, m_LineNr, m_ColNr, m_Token );
	}
}
