#include "QLibPCH.h"
#include "../../Common/Value.h"
#include "../../Common/Chunk.h"
#include "../Compiler.h"
#include "AST.h"

namespace Compiler
{
	bool TypeCheck( uint32_t targetType, uint32_t exprType, bool strict )
	{
		if ( targetType == TYPE_UNKNOWN || exprType == TYPE_UNKNOWN )
			return true;

		if ( !strict && ( targetType & TYPE_NULL ) )
			return true;

		// Strict type checking
		return targetType == exprType;
	}

	std::string TypeToString( uint32_t type )
	{
		if ( type == TYPE_UNKNOWN )
			return "unknown";

		std::string result = "";

		std::map< CompileTypeInfo, std::string > typeStrings ={
			{ TYPE_NULL, 			"null" },
			{ TYPE_NUMBER, 			"number" },
			{ TYPE_BOOL, 			"bool" },
			{ TYPE_TABLE, 			"Table" },
			{ TYPE_FUNCTION, 		"function" },
			{ TYPE_NATIVE, 			"native" },
			{ TYPE_STRING, 			"string" },
			{ TYPE_NONE, 			"none" },
		};

		for ( auto typeString : typeStrings )
		{
			if ( type & typeString.first )
			{
				if ( result.length() > 0 )
					result += " | " + typeString.second;
				else
					result = typeString.second;
			}
		}

		return result.length() > 0 ? result : "not_supported";
	}

	uint32_t ResolveReturnType( const ListNode* funcNode, Assembler& assembler )
	{
		// Is there an explicitly defined return type?
		auto retnTypeNode = static_cast< ValueNode* >( funcNode->GetList()[ 2 ] );
		uint32_t retnType = ( uint32_t ) AS_NUMBER( retnTypeNode->GetValue() );

		// Type shouldn't be deduced by compiler ?
		if ( !( retnType & TYPE_AUTO ) )
			return retnType;

		// Combine all return statement types
		uint32_t returnTypes = TYPE_NULL;

		auto argsList = ParseArgsList( static_cast< ListNode* >( funcNode->GetList()[ 0 ] ) );

		for ( auto arg : argsList )
			assembler.AddArgument( arg.m_Name, true, arg.m_Type, TYPE_UNKNOWN );

		std::function< void( const BaseNode* ) > visitNode;
		visitNode = [ &visitNode, &returnTypes, &assembler ]( const BaseNode* node ) -> void
		{
			if ( !node )
				return;

			switch ( node->Type() )
			{
			case NT_TERM:
			{
				if ( node->Id() == NODE_RETURN )
					returnTypes |= TYPE_NULL;

				break;
			}
			case NT_SIMPLE:
			{
				auto simple = static_cast< const SimpleNode* >( node );

				if ( simple->Id() == NODE_RETURN )
					returnTypes |= simple->GetNode()->ExprType( assembler );
				else
					visitNode( node );

				break;
			}
			case NT_VALUE:
				break;
			case NT_COMPLEX:
			{
				auto complex = static_cast< const ComplexNode* >( node );

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
		return returnTypes;
	}

	uint32_t ValueNode::ExprType( Assembler& assembler ) const
	{
		switch ( m_NodeId )
		{
		case NODE_NAME:
		{
			uint32_t nameIndex;
			auto name = AS_STRING( m_Value )->GetString();

			Assembler::Variable_t varInfo;
			if ( assembler.FindArgument( name, &varInfo ) )
				return varInfo.m_Type;

			if ( assembler.FindLocal( name, &nameIndex, &varInfo ) )
				return varInfo.m_Type;

			if ( assembler.FindUpvalue( name, &nameIndex, &varInfo ) )
				return varInfo.m_Type;

			if ( assembler.FindGlobal( name, &varInfo ) )
				return varInfo.m_Type;

			return TYPE_NONE;
		}
		case NODE_CONSTANT:
		{
			if ( IS_NUMBER( m_Value ) ) return TYPE_NUMBER;
			else if ( IS_NULL( m_Value ) ) return TYPE_NULL;
			else if ( IS_BOOL( m_Value ) ) return TYPE_BOOL;
			else if ( IS_OBJECT( m_Value ) )
			{
				switch ( AS_OBJECT( m_Value )->m_Type )
				{
				case QScript::OT_TABLE: return TYPE_TABLE;
				case QScript::OT_CLOSURE: return TYPE_CLOSURE;
				case QScript::OT_FUNCTION: return TYPE_FUNCTION;
				case QScript::OT_NATIVE: return TYPE_NATIVE;
				case QScript::OT_STRING: return TYPE_STRING;
				case QScript::OT_UPVALUE: return TYPE_UPVALUE;
				default: break;
				}
			}
			return TYPE_UNKNOWN;
		}
		default:
			throw CompilerException( "cp_invalid_value_node", "Unknown value node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}
	}

	uint32_t ComplexNode::ExprType( Assembler& assembler ) const
	{
		switch ( m_NodeId )
		{
		case NODE_ACCESS_PROP:
		{
			// TODO: Resolve table property types compile-time
			return TYPE_UNKNOWN;
		}
		case NODE_ACCESS_ARRAY:
		{
			// TODO: Compile-time arrays?
			return TYPE_UNKNOWN;
		}
		case NODE_ASSIGN: return m_Right->ExprType( assembler );
		case NODE_ASSIGNADD: return m_Right->ExprType( assembler );
		case NODE_ASSIGNDIV: return TYPE_NUMBER;
		case NODE_ASSIGNMOD: return TYPE_NUMBER;
		case NODE_ASSIGNMUL: return TYPE_NUMBER;
		case NODE_ASSIGNSUB: return TYPE_NUMBER;
		case NODE_CALL:
		{
			// Return type
			switch ( m_Left->Id() )
			{
			case NODE_FUNC:
				return ResolveReturnType( static_cast< ListNode* >( m_Left ), assembler );
			case NODE_NAME:
			{
				auto nameValue = static_cast< ValueNode* >( m_Left )->GetValue();
				auto name = AS_STRING( nameValue )->GetString();

				uint32_t nameIndex;
				Assembler::Variable_t varInfo;

				if ( assembler.FindLocal( name, &nameIndex, &varInfo ) )
					return varInfo.m_ReturnType;

				if ( assembler.FindUpvalue( name, &nameIndex, &varInfo ) )
					return varInfo.m_ReturnType;

				if ( assembler.FindGlobal( name, &varInfo ) )
					return varInfo.m_ReturnType;

				break;
			}
			default:
				break;
			}

			return TYPE_UNKNOWN;
		}
		case NODE_AND: return m_Left->ExprType( assembler ) | m_Right->ExprType( assembler );
		case NODE_OR: return m_Left->ExprType( assembler ) | m_Right->ExprType( assembler );
		case NODE_DEC: return TYPE_NUMBER;
		case NODE_INC: return TYPE_NUMBER;
		case NODE_ADD:
		{
			auto leftType = m_Left->ExprType( assembler );
			auto rightType = m_Right->ExprType( assembler );

			if ( ( leftType & TYPE_STRING ) || ( rightType & TYPE_STRING ) )
				return TYPE_STRING;

			return TYPE_NUMBER;
		}
		case NODE_SUB: return TYPE_NUMBER;
		case NODE_MUL: return TYPE_NUMBER;
		case NODE_DIV: return TYPE_NUMBER;
		case NODE_MOD: return TYPE_NUMBER;
		case NODE_POW: return TYPE_NUMBER;
		case NODE_EQUALS: return TYPE_BOOL;
		case NODE_NOTEQUALS: return TYPE_BOOL;
		case NODE_GREATERTHAN: return TYPE_BOOL;
		case NODE_GREATEREQUAL: return TYPE_BOOL;
		case NODE_LESSTHAN: return TYPE_BOOL;
		case NODE_LESSEQUAL: return TYPE_BOOL;
		default:
			throw CompilerException( "cp_invalid_complex_node", "Unknown complex node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}
	}

	uint32_t SimpleNode::ExprType( Assembler& assembler ) const
	{
		switch ( m_NodeId )
		{
		case NODE_IMPORT: return TYPE_NONE;
		case NODE_RETURN: return m_Node->ExprType( assembler );
		case NODE_NOT: return TYPE_BOOL;
		case NODE_NEG: return TYPE_NUMBER;
		default:
			throw CompilerException( "cp_invalid_simple_node", "Unknown simple node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}
	}

	uint32_t ListNode::ExprType( Assembler& assembler ) const
	{
		switch ( m_NodeId )
		{
		case NODE_TABLE: return TYPE_NONE;
		case NODE_ARRAY: return TYPE_NONE;
		case NODE_DO: return TYPE_NONE;
		case NODE_FOR: return TYPE_NONE;
		case NODE_FUNC: return TYPE_FUNCTION;
		case NODE_IF: return TYPE_NONE;
		case NODE_SCOPE: return TYPE_NONE;
		case NODE_WHILE: return TYPE_NONE;
		case NODE_INLINE_IF:
		{
			return m_NodeList[ 1 ]->ExprType( assembler ) | m_NodeList[ 2 ]->ExprType( assembler );
		}
		case NODE_CONSTVAR:
		{
			auto& varTypeValue = static_cast< ValueNode* >( m_NodeList[ 2 ] )->GetValue();
			auto assignedType = ( uint32_t ) AS_NUMBER( varTypeValue );

			if ( assignedType != TYPE_UNKNOWN && !( assignedType & TYPE_AUTO ) )
				return assignedType;

			if ( !m_NodeList[ 1 ] )
				return TYPE_UNKNOWN;

			return m_NodeList[ 1 ]->ExprType( assembler );
		}
		case NODE_VAR:
		{
			auto& varTypeValue = static_cast< ValueNode* >( m_NodeList[ 2 ] )->GetValue();
			auto type = ( uint32_t ) AS_NUMBER( varTypeValue );

			if ( ( type & TYPE_AUTO ) && m_NodeList[ 1 ] )
				return m_NodeList[ 1 ]->ExprType( assembler );

			return type;
		}
		default:
			throw CompilerException( "cp_invalid_list_node", "Unknown list node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}
	}
}
