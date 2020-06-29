#include "QLibPCH.h"
#include "../../Common/Value.h"
#include "../../Common/Chunk.h"
#include "../Compiler.h"
#include "AST.h"
#include "Typing.h"

#define EXPR_TYPE_STATIC( type ) (m_ExprType->m_Bits = type)
#define EXPR_TYPE_COPY( otherType ) DeepCopyType( otherType, m_ExprType )
#define EXPR_TYPE_FROM_CHILD( child ) DeepCopyType( *child->ExprType( assembler ), m_ExprType )

namespace Compiler
{
	void ResolveReturnType( ListNode* funcNode, Assembler& assembler, Type_t* out )
	{
		// Is there an explicitly defined return type?
		const Type_t* returnType = static_cast< TypeNode* >( funcNode->GetList()[ 2 ] )->GetType();

		// Type shouldn't be deduced by compiler ?
		if ( !returnType->IsAuto() )
		{
			DeepCopyType( *returnType, out );
			return;
		}

		// Combine all return statement types
		Type_t returnTypes = Type_t( TYPE_NULL );

		// Is first return?
		bool firstReturn = true;

		auto argsList = ParseArgsList( static_cast< ListNode* >( funcNode->GetList()[ 0 ] ), assembler );

		for ( auto arg : argsList )
		{
			assembler.AddArgument( arg.m_Name, true, arg.m_LineNr, arg.m_ColNr, arg.m_Type );
			FreeTypes( arg.m_Type );
		}

		std::function< void( BaseNode* ) > visitNode;
		visitNode = [ &visitNode, &returnTypes, &firstReturn, &assembler ]( BaseNode* node ) -> void
		{
			if ( !node || returnTypes.IsUnknown() )
				return;

			switch ( node->Type() )
			{
			case NT_TERM:
			{
				if ( node->Id() == NODE_RETURN )
				{
					returnTypes.m_Bits |= TYPE_NULL;
					firstReturn = false;
				}

				break;
			}
			case NT_SIMPLE:
			{
				auto simple = static_cast< SimpleNode* >( node );

				if ( simple->Id() == NODE_RETURN )
				{
					if ( firstReturn )
					{
						// Reset return type to 0, since there is at least 1 return, we can discard the default null -type
						returnTypes = 0;
						firstReturn = false;
					}

					auto returnExpressionType = simple->GetNode()->ExprType( assembler );

					if ( returnExpressionType->IsUnknown() )
						returnTypes.m_Bits = TYPE_UNKNOWN;
					else
						returnTypes.Join( returnExpressionType ); // TODO: Error if type join fails
				}
				else
				{
					visitNode( node );
				}

				break;
			}
			case NT_VALUE:
				break;
			case NT_COMPLEX:
			{
				auto complex = static_cast< ComplexNode* >( node );

				visitNode( complex->GetLeft() );
				visitNode( complex->GetRight() );

				break;
			}
			case NT_LIST:
			{
				auto list = static_cast< const ListNode* >( node );

				for ( auto subNode : list->GetList() )
					visitNode( subNode );

				break;
			}
			default:
				break;
			}
		};

		visitNode( funcNode );

		assembler.ClearArguments();

		DeepCopyType( returnTypes, out );
	}

	const Type_t* TermNode::ExprType( Assembler& assembler )
	{
		switch ( m_NodeId )
		{
		case NODE_RETURN: EXPR_TYPE_STATIC( TYPE_NULL ); break;
		default:
			throw CompilerException( "cp_invalid_term_node", "Unknown term node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}

		return m_ExprType;
	}

	const Type_t* ValueNode::ExprType( Assembler& assembler )
	{
		switch ( m_NodeId )
		{
		case NODE_NAME:
		{
			uint32_t nameIndex;
			auto name = AS_STRING( m_Value )->GetString();

			Variable_t varInfo;
			if ( assembler.FindArgument( name, &varInfo ) )
				EXPR_TYPE_COPY( *varInfo.m_Type );
			else if ( assembler.FindLocal( name, &nameIndex, &varInfo ) )
				EXPR_TYPE_COPY( *varInfo.m_Type );
			else if ( assembler.FindUpvalue( name, &nameIndex, &varInfo ) )
				EXPR_TYPE_COPY( *varInfo.m_Type );
			else if ( assembler.FindGlobal( name, &varInfo ) )
				EXPR_TYPE_COPY( *varInfo.m_Type );
			else
				EXPR_TYPE_STATIC( TYPE_NONE );

			break;
		}
		case NODE_CONSTANT:
		{
			if ( IS_NUMBER( m_Value ) ) EXPR_TYPE_STATIC( TYPE_NUMBER );
			else if ( IS_NULL( m_Value ) ) EXPR_TYPE_STATIC( TYPE_NULL );
			else if ( IS_BOOL( m_Value ) ) EXPR_TYPE_STATIC( TYPE_BOOL );
			else if ( IS_OBJECT( m_Value ) )
			{
				switch ( AS_OBJECT( m_Value )->m_Type )
				{
				case QScript::OT_TABLE: EXPR_TYPE_STATIC( TYPE_TABLE ); break;
				case QScript::OT_CLOSURE: EXPR_TYPE_STATIC( TYPE_CLOSURE ); break;
				case QScript::OT_FUNCTION: EXPR_TYPE_STATIC( TYPE_FUNCTION ); break;
				case QScript::OT_NATIVE: EXPR_TYPE_STATIC( TYPE_NATIVE ); break;
				case QScript::OT_STRING: EXPR_TYPE_STATIC( TYPE_STRING ); break;
				case QScript::OT_UPVALUE: EXPR_TYPE_STATIC( TYPE_UPVALUE ); break;
				default: break;
				}
			}
			else
			{
				EXPR_TYPE_STATIC( TYPE_UNKNOWN );
			}

			break;
		}
		default:
			throw CompilerException( "cp_invalid_value_node", "Unknown value node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}

		return m_ExprType;
	}

	const Type_t* ComplexNode::ExprType( Assembler& assembler )
	{
		switch ( m_NodeId )
		{
		case NODE_ACCESS_PROP:
		{
			// TODO: Resolve table property types compile-time
			EXPR_TYPE_STATIC( TYPE_UNKNOWN );
			break;
		}
		case NODE_ACCESS_ARRAY:
		{
			// TODO: Compile-time arrays?
			EXPR_TYPE_STATIC( TYPE_UNKNOWN );
			break;
		}
		case NODE_ASSIGN: EXPR_TYPE_FROM_CHILD( m_Right ); break;
		case NODE_ASSIGNADD: EXPR_TYPE_FROM_CHILD( m_Right ); break;
		case NODE_ASSIGNDIV: EXPR_TYPE_STATIC( TYPE_NUMBER ); break;
		case NODE_ASSIGNMOD: EXPR_TYPE_STATIC( TYPE_NUMBER ); break;
		case NODE_ASSIGNMUL: EXPR_TYPE_STATIC( TYPE_NUMBER ); break;
		case NODE_ASSIGNSUB: EXPR_TYPE_STATIC( TYPE_NUMBER ); break;
		case NODE_CALL:
		{
			// Return type
			switch ( m_Left->Id() )
			{
			case NODE_FUNC:
			{
				ResolveReturnType( static_cast< ListNode* >( m_Left ), assembler, m_ExprType );
				break;
			}
			case NODE_NAME:
			{
				auto nameValue = static_cast< ValueNode* >( m_Left )->GetValue();
				auto name = AS_STRING( nameValue )->GetString();

				uint32_t nameIndex;
				Variable_t varInfo;

				if ( assembler.FindLocal( name, &nameIndex, &varInfo )
					|| assembler.FindUpvalue( name, &nameIndex, &varInfo )
					|| assembler.FindGlobal( name, &varInfo ) )
				{
					// Identifier has return type?
					if ( varInfo.m_Type->m_ReturnType )
						EXPR_TYPE_COPY( *varInfo.m_Type->m_ReturnType );
					else if ( varInfo.m_Type->IsUnknown() )
						EXPR_TYPE_STATIC( TYPE_UNKNOWN );
					else
						EXPR_TYPE_STATIC( TYPE_NONE );
				}
				else
				{
					// Identifier not found
					EXPR_TYPE_STATIC( TYPE_NONE );
				}

				break;
			}
			default:
				EXPR_TYPE_STATIC( TYPE_UNKNOWN );
				break;
			}

			break;
		}
		case NODE_AND:
		{
			EXPR_TYPE_FROM_CHILD( m_Left );
			m_ExprType->Join( m_Right->ExprType( assembler ) );
			break;
		}
		case NODE_OR:
		{
			EXPR_TYPE_FROM_CHILD( m_Left );
			m_ExprType->Join( m_Right->ExprType( assembler ) );
			break;
		}
		case NODE_DEC: EXPR_TYPE_STATIC( TYPE_NUMBER ); break;
		case NODE_INC: EXPR_TYPE_STATIC( TYPE_NUMBER ); break;
		case NODE_ADD:
		{
			auto leftType = m_Left->ExprType( assembler );
			auto rightType = m_Right->ExprType( assembler );

			if ( Type_t( TYPE_STRING ).DeepEquals( leftType ) )
				EXPR_TYPE_STATIC( TYPE_STRING );
			else if ( Type_t( TYPE_STRING ).DeepEquals( rightType ) )
				EXPR_TYPE_STATIC( TYPE_STRING );
			else if ( leftType->IsUnknown() || rightType->IsUnknown() )
				EXPR_TYPE_STATIC( TYPE_UNKNOWN );
			else
				EXPR_TYPE_STATIC( TYPE_NUMBER );

			break;
		}
		case NODE_SUB: EXPR_TYPE_STATIC( TYPE_NUMBER ); break;
		case NODE_MUL: EXPR_TYPE_STATIC( TYPE_NUMBER );	break;
		case NODE_DIV: EXPR_TYPE_STATIC( TYPE_NUMBER );	break;
		case NODE_MOD: EXPR_TYPE_STATIC( TYPE_NUMBER );	break;
		case NODE_POW: EXPR_TYPE_STATIC( TYPE_NUMBER );	break;
		case NODE_EQUALS: EXPR_TYPE_STATIC( TYPE_BOOL ); break;
		case NODE_NOTEQUALS: EXPR_TYPE_STATIC( TYPE_BOOL ); break;
		case NODE_GREATERTHAN: EXPR_TYPE_STATIC( TYPE_BOOL ); break;
		case NODE_GREATEREQUAL: EXPR_TYPE_STATIC( TYPE_BOOL ); break;
		case NODE_LESSTHAN: EXPR_TYPE_STATIC( TYPE_BOOL ); break;
		case NODE_LESSEQUAL: EXPR_TYPE_STATIC( TYPE_BOOL ); break;
		default:
			throw CompilerException( "cp_invalid_complex_node", "Unknown complex node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}

		return m_ExprType;
	}

	const Type_t* SimpleNode::ExprType( Assembler& assembler )
	{
		switch ( m_NodeId )
		{
		case NODE_IMPORT: EXPR_TYPE_STATIC( TYPE_NONE ); break;
		case NODE_RETURN: EXPR_TYPE_FROM_CHILD( m_Node ); break;
		case NODE_NOT: EXPR_TYPE_STATIC( TYPE_BOOL ); break;
		case NODE_NEG: EXPR_TYPE_STATIC( TYPE_NUMBER ); break;
		default:
			throw CompilerException( "cp_invalid_simple_node", "Unknown simple node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}

		return m_ExprType;
	}

	const Type_t* ListNode::ExprType( Assembler& assembler )
	{
		switch ( m_NodeId )
		{
		case NODE_TABLE: EXPR_TYPE_STATIC( TYPE_TABLE ); break;
		case NODE_ARRAY: EXPR_TYPE_STATIC( TYPE_ARRAY ); break;
		case NODE_FUNC: EXPR_TYPE_STATIC( TYPE_FUNCTION ); break;
		case NODE_INLINE_IF:
		{
			EXPR_TYPE_FROM_CHILD( m_NodeList[ 1 ] );
			m_ExprType->Join( m_NodeList[ 2 ]->ExprType( assembler ) );
			break;
		}
		case NODE_CONSTVAR:
		{
			const Type_t* varType = static_cast< TypeNode* >( m_NodeList[ 2 ] )->GetType();

			if ( !varType->IsAuto() )
				EXPR_TYPE_COPY( *varType ); // "const num x = ..."
			else if ( !m_NodeList[ 1 ] )
				EXPR_TYPE_STATIC( TYPE_NULL ); // "const x;", "const auto x;"

			// "const auto x = ..."
			EXPR_TYPE_FROM_CHILD( m_NodeList[ 1 ] );
			break;
		}
		case NODE_VAR:
		{
			const Type_t* varType = static_cast< TypeNode* >( m_NodeList[ 2 ] )->GetType();

			if ( varType->IsAuto() && m_NodeList[ 1 ] )
				EXPR_TYPE_FROM_CHILD( m_NodeList[ 1 ] );
			else
				EXPR_TYPE_COPY( *varType );

			break;
		}
		case NODE_DO:
		case NODE_FOR:
		case NODE_IF:
		case NODE_SCOPE:
		case NODE_WHILE:
			EXPR_TYPE_STATIC( TYPE_NONE );
			break;
		default:
			throw CompilerException( "cp_invalid_list_node", "Unknown list node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}

		return m_ExprType;
	}

	const Type_t* TypeNode::ExprType( Assembler& assembler )
	{
		throw CompilerException( "cp_invalid_type_node", "Can not get type of a type node. Call GetType() instead", m_LineNr, m_ColNr, m_Token );
	}
}
