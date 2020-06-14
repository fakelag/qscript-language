#include "QLibPCH.h"
#include "../../Common/Value.h"
#include "../../Common/Chunk.h"
#include "../Compiler.h"
#include "Typing.h"
#include "AST.h"

namespace Compiler
{
	std::vector< const Type_t* > Type_t::m_Types = {};

	Type_t* Type_t::AddReturnType( uint32_t bits )
	{
		auto type = QS_NEW Type_t( bits );
		m_ReturnType = type;
		return type;
	}

	Type_t* Type_t::AddArgument( const std::string& name, uint32_t bits )
	{
		assert( m_Bits & TYPE_FUNCTION == TYPE_FUNCTION );
		m_ArgTypes.push_back( std::make_pair( name, QS_NEW Type_t( bits ) ) );
		return m_ArgTypes.back().second;
	}

	Type_t* Type_t::AddProperty( const std::string& name, uint32_t bits )
	{
		assert( m_Bits & TYPE_TABLE == TYPE_TABLE );
		m_PropTypes.push_back( std::make_pair( name, QS_NEW Type_t( bits ) ) );
		return m_PropTypes.back().second;
	}

	Type_t* Type_t::AddIndice( uint32_t bits )
	{
		assert( m_Bits & TYPE_ARRAY == TYPE_ARRAY );
		m_IndiceTypes.push_back( QS_NEW Type_t( bits ) );
		return m_IndiceTypes.back();
	}

	size_t SaveTypes( const Type_t* types )
	{
		if ( !types )
			return NULL;

		Type_t::m_Types.push_back( types );
		return Type_t::m_Types.size() - 1;
	}

	const Type_t* LoadTypes( size_t index )
	{
		return Type_t::m_Types[ index ];
	}

	void FreeTypes( const Type_t* types )
	{
		if ( !types )
			return;

		FreeTypes( types->m_ReturnType );

		for ( auto arg : types->m_ArgTypes )
			FreeTypes( arg.second );

		for ( auto prop : types->m_PropTypes )
			FreeTypes( prop.second );

		for ( auto indice : types->m_IndiceTypes )
			FreeTypes( indice );

		delete types;
	}

	bool TypeCheck( Type_t targetType, Type_t exprType )
	{
		if ( targetType.IsUnknown() || exprType.IsUnknown() )
			return true;

		// Strict type checking
		return targetType == exprType;
	}

	std::string TypeToString( Type_t type )
	{
		if ( type.IsUnknown() )
			return "unknown";

		std::string result = "";

		std::map< CompileTypeBits, std::string > typeStrings ={
			{ TYPE_NULL, 			"null" },
			{ TYPE_NUMBER, 			"num" },
			{ TYPE_BOOL, 			"bool" },
			{ TYPE_TABLE, 			"Table" },
			{ TYPE_ARRAY, 			"Array" },
			{ TYPE_FUNCTION, 		"function" },
			{ TYPE_NATIVE, 			"native" },
			{ TYPE_STRING, 			"string" },
			{ TYPE_NONE, 			"none" },
		};

		for ( auto typeString : typeStrings )
		{
			if ( type.m_Bits & typeString.first )
			{
				if ( result.length() > 0 )
					result += " | " + typeString.second;
				else
					result = typeString.second;
			}
		}

		return result.length() > 0 ? result : "not_supported";
	}

	Type_t ResolveReturnType( const ListNode* funcNode, Assembler& assembler )
	{
		// Is there an explicitly defined return type?
		auto retnTypeNode = static_cast< ValueNode* >( funcNode->GetList()[ 2 ] );
		uint32_t retnType = ( uint32_t ) AS_NUMBER( retnTypeNode->GetValue() );

		// Type shouldn't be deduced by compiler ?
		if ( !( retnType & TYPE_AUTO ) )
			return retnType;

		// Combine all return statement types
		Type_t returnTypes = Type_t( TYPE_NULL );

		// Is first return?
		bool firstReturn = true;

		auto argsList = ParseArgsList( static_cast< ListNode* >( funcNode->GetList()[ 0 ] ) );

		for ( auto arg : argsList )
			assembler.AddArgument( arg.m_Name, true, arg.m_LineNr, arg.m_ColNr, arg.m_Type );

		std::function< void( const BaseNode* ) > visitNode;
		visitNode = [ &visitNode, &returnTypes, &firstReturn, &assembler ]( const BaseNode* node ) -> void
		{
			if ( !node || returnTypes.m_Bits == TYPE_UNKNOWN )
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
				auto simple = static_cast< const SimpleNode* >( node );

				if ( simple->Id() == NODE_RETURN )
				{
					if ( firstReturn )
					{
						// Reset return type to 0, since there is at least 1 return, we can discard the default null -type
						returnTypes = 0;
						firstReturn = false;
					}

					auto returnExpressionType = simple->GetNode()->ExprType( assembler );

					if ( returnExpressionType.m_Bits == TYPE_UNKNOWN )
						returnTypes.m_Bits = TYPE_UNKNOWN;
					else
						returnTypes.m_Bits |= returnExpressionType.m_Bits;
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

	Type_t ValueNode::ExprType( Assembler& assembler ) const
	{
		switch ( m_NodeId )
		{
		case NODE_NAME:
		{
			uint32_t nameIndex;
			auto name = AS_STRING( m_Value )->GetString();

			Variable_t varInfo;
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
			if ( IS_NUMBER( m_Value ) ) return Type_t( TYPE_NUMBER );
			else if ( IS_NULL( m_Value ) ) return Type_t( TYPE_NULL );
			else if ( IS_BOOL( m_Value ) ) return Type_t( TYPE_BOOL );
			else if ( IS_OBJECT( m_Value ) )
			{
				switch ( AS_OBJECT( m_Value )->m_Type )
				{
				case QScript::OT_TABLE: return Type_t( TYPE_TABLE );
				case QScript::OT_CLOSURE: return Type_t( TYPE_CLOSURE );
				case QScript::OT_FUNCTION: return Type_t( TYPE_FUNCTION );
				case QScript::OT_NATIVE: return Type_t( TYPE_NATIVE );
				case QScript::OT_STRING: return Type_t( TYPE_STRING );
				case QScript::OT_UPVALUE: return Type_t( TYPE_UPVALUE );
				default: break;
				}
			}
			return Type_t( TYPE_UNKNOWN );
		}
		default:
			throw CompilerException( "cp_invalid_value_node", "Unknown value node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}
	}

	Type_t ComplexNode::ExprType( Assembler& assembler ) const
	{
		switch ( m_NodeId )
		{
		case NODE_ACCESS_PROP:
		{
			// TODO: Resolve table property types compile-time
			return Type_t( TYPE_UNKNOWN );
		}
		case NODE_ACCESS_ARRAY:
		{
			// TODO: Compile-time arrays?
			return Type_t( TYPE_UNKNOWN );
		}
		case NODE_ASSIGN: return m_Right->ExprType( assembler );
		case NODE_ASSIGNADD: return m_Right->ExprType( assembler );
		case NODE_ASSIGNDIV: return Type_t( TYPE_NUMBER );
		case NODE_ASSIGNMOD: return Type_t( TYPE_NUMBER );
		case NODE_ASSIGNMUL: return Type_t( TYPE_NUMBER );
		case NODE_ASSIGNSUB: return Type_t( TYPE_NUMBER );
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
				Variable_t varInfo;

				if ( assembler.FindLocal( name, &nameIndex, &varInfo ) )
					return varInfo.m_Type.m_ReturnType ? *varInfo.m_Type.m_ReturnType : Type_t( TYPE_NONE );

				if ( assembler.FindUpvalue( name, &nameIndex, &varInfo ) )
					return varInfo.m_Type.m_ReturnType ? *varInfo.m_Type.m_ReturnType : Type_t( TYPE_NONE );

				if ( assembler.FindGlobal( name, &varInfo ) )
					return varInfo.m_Type.m_ReturnType ? *varInfo.m_Type.m_ReturnType : Type_t( TYPE_NONE );

				break;
			}
			default:
				break;
			}

			return TYPE_UNKNOWN;
		}
		case NODE_AND: return m_Left->ExprType( assembler ) | m_Right->ExprType( assembler );
		case NODE_OR: return m_Left->ExprType( assembler ) | m_Right->ExprType( assembler );
		case NODE_DEC: return Type_t( TYPE_NUMBER );
		case NODE_INC: return Type_t( TYPE_NUMBER );
		case NODE_ADD:
		{
			auto leftType = m_Left->ExprType( assembler );
			auto rightType = m_Right->ExprType( assembler );

			if ( ( leftType & TYPE_STRING ) || ( rightType & TYPE_STRING ) )
				return TYPE_STRING;

			if ( leftType.IsUnknown() )
				return leftType;

			if ( rightType.IsUnknown() )
				return rightType;

			return TYPE_NUMBER;
		}
		case NODE_SUB: return Type_t( TYPE_NUMBER );
		case NODE_MUL: return Type_t( TYPE_NUMBER );
		case NODE_DIV: return Type_t( TYPE_NUMBER );
		case NODE_MOD: return Type_t( TYPE_NUMBER );
		case NODE_POW: return Type_t( TYPE_NUMBER );
		case NODE_EQUALS: return Type_t( TYPE_BOOL );
		case NODE_NOTEQUALS: return Type_t( TYPE_BOOL );
		case NODE_GREATERTHAN: return Type_t( TYPE_BOOL );
		case NODE_GREATEREQUAL: return Type_t( TYPE_BOOL );
		case NODE_LESSTHAN: return Type_t( TYPE_BOOL );
		case NODE_LESSEQUAL: return Type_t( TYPE_BOOL );
		default:
			throw CompilerException( "cp_invalid_complex_node", "Unknown complex node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}
	}

	Type_t SimpleNode::ExprType( Assembler& assembler ) const
	{
		switch ( m_NodeId )
		{
		case NODE_IMPORT: return Type_t( TYPE_NONE );
		case NODE_RETURN: return m_Node->ExprType( assembler );
		case NODE_NOT: return Type_t( TYPE_BOOL );
		case NODE_NEG: return Type_t( TYPE_NUMBER );
		default:
			throw CompilerException( "cp_invalid_simple_node", "Unknown simple node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}
	}

	Type_t ListNode::ExprType( Assembler& assembler ) const
	{
		switch ( m_NodeId )
		{
		case NODE_TABLE: return Type_t( TYPE_TABLE );
		case NODE_ARRAY: return Type_t( TYPE_ARRAY );
		case NODE_DO: return Type_t( TYPE_NONE );
		case NODE_FOR: return Type_t( TYPE_NONE );
		case NODE_FUNC: return Type_t( TYPE_FUNCTION );
		case NODE_IF: return Type_t( TYPE_NONE );
		case NODE_SCOPE: return Type_t( TYPE_NONE );
		case NODE_WHILE: return Type_t( TYPE_NONE );
		case NODE_INLINE_IF:
		{
			return m_NodeList[ 1 ]->ExprType( assembler ) | m_NodeList[ 2 ]->ExprType( assembler );
		}
		case NODE_CONSTVAR:
		{
			auto& varTypeValue = static_cast< ValueNode* >( m_NodeList[ 2 ] )->GetValue();
			auto assignedType = ( uint32_t ) AS_NUMBER( varTypeValue );

			if ( assignedType != TYPE_UNKNOWN && !( assignedType & TYPE_AUTO ) )
				return Type_t( assignedType );

			if ( !m_NodeList[ 1 ] )
				return Type_t( TYPE_UNKNOWN );

			return m_NodeList[ 1 ]->ExprType( assembler );
		}
		case NODE_VAR:
		{
			auto& varTypeValue = static_cast< ValueNode* >( m_NodeList[ 2 ] )->GetValue();
			auto type = ( uint32_t ) AS_NUMBER( varTypeValue );

			if ( ( type & TYPE_AUTO ) && m_NodeList[ 1 ] )
				return m_NodeList[ 1 ]->ExprType( assembler );

			return Type_t( type );
		}
		default:
			throw CompilerException( "cp_invalid_list_node", "Unknown list node: " + std::to_string( m_NodeId ), m_LineNr, m_ColNr, m_Token );
		}
	}
}
