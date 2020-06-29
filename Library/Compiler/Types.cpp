#include "QLibPCH.h"
#include "Types.h"

namespace Compiler
{
	bool FunctionTypeEqualStrict( const Type_t* a, const Type_t* b )
	{
		// Are return types the same?
		if ( !a->m_ReturnType->DeepEquals( b->m_ReturnType ) )
			return false;

		// Same number of arguments?
		if ( a->m_ArgTypes.size() != b->m_ArgTypes.size() )
			return false;

		// Same argument types?
		for ( size_t i = 0; i < a->m_ArgTypes.size(); ++i )
		{
			auto arg = a->m_ArgTypes[ i ].second;
			auto exprArg = b->m_ArgTypes[ i ].second;

			if ( !arg->DeepEquals( exprArg ) )
				return false;
		}

		return true;
	}

	bool TableTypeEqualStrict( const Type_t* a, const Type_t* b )
	{
		// Same number of properties?
		if ( a->m_PropTypes.size() != b->m_PropTypes.size() )
			return false;

		// Same property names & types?
		for ( auto prop : a->m_PropTypes )
		{
			auto exprProp = std::find_if( b->m_PropTypes.begin(), b->m_PropTypes.end(), [ &prop ]( const NamedType_t& type ) {
				return type.first == prop.first;
			} );

			// Was the property found?
			if ( exprProp == b->m_PropTypes.end() )
				return false;

			// Property types differ?
			if ( !prop.second->DeepEquals( exprProp->second ) )
				return false;
		}

		return true;
	}

	bool ArrayTypeEqualStrict( const Type_t* a, const Type_t* b )
	{
		// Same array indice types?
		for ( size_t i = 0; i < a->m_IndiceTypes.size(); ++i )
		{
			auto indice = a->m_IndiceTypes[ i ];
			auto exprIndice = b->m_IndiceTypes[ i ];

			if ( !indice->DeepEquals( exprIndice ) )
				return false;
		}

		return true;
	}

	void DeepCopyFunctionType( const Type_t& from, Type_t* to )
	{
		// Copy return type
		to->m_ReturnType = DeepCopyType( *from.m_ReturnType, QS_NEW Type_t );

		// Copy arguments
		std::transform( from.m_ArgTypes.begin(), from.m_ArgTypes.end(), std::back_inserter( to->m_ArgTypes ), []( const NamedType_t& argument ) {
			return std::make_pair( argument.first, DeepCopyType( *argument.second, QS_NEW Type_t ) );
		} );
	}

	void DeepCopyTableType( const Type_t& from, Type_t* to )
	{
		// Copy properties
		std::transform( from.m_PropTypes.begin(), from.m_PropTypes.end(), std::back_inserter( to->m_PropTypes ), []( const NamedType_t& prop ) {
			return std::make_pair( prop.first, DeepCopyType( *prop.second, QS_NEW Type_t ) );
		} );
	}

	void DeepCopyArrayType( const Type_t& from, Type_t* to )
	{
		// Copy array indices
		std::transform( from.m_IndiceTypes.begin(), from.m_IndiceTypes.end(), std::back_inserter( to->m_IndiceTypes ), []( const Type_t* indice ) {
			return DeepCopyType( *indice, QS_NEW Type_t );
		} );
	}

	bool Type_t::IsAssignable( const Type_t* exprType ) const
	{
		if ( IsUnknown() || exprType->IsUnknown() )
			return true;

		// Require types to overlap
		if ( ( m_Bits & exprType->m_Bits ) == 0 )
			return false;

		if ( exprType->m_Bits & TYPE_FUNCTION )
		{
			if ( !FunctionTypeEqualStrict( this, exprType ) )
				return false;
		}

		if ( exprType->m_Bits & TYPE_TABLE )
		{
			if ( !TableTypeEqualStrict( this, exprType ) )
				return false;
		}

		if ( exprType->m_Bits & TYPE_ARRAY )
		{
			if ( !ArrayTypeEqualStrict( this, exprType ) )
				return false;
		}

		return true;
	}

	bool Type_t::DeepEquals( const Type_t* other ) const
	{
		// Require types to be same
		if ( m_Bits != other->m_Bits )
			return false;

		if ( other->m_Bits & TYPE_FUNCTION )
		{
			if ( !FunctionTypeEqualStrict( this, other ) )
				return false;
		}

		if ( other->m_Bits & TYPE_TABLE )
		{
			if ( !TableTypeEqualStrict( this, other ) )
				return false;
		}

		if ( other->m_Bits & TYPE_ARRAY )
		{
			if ( !ArrayTypeEqualStrict( this, other ) )
				return false;
		}

		return true;
	}

	bool Type_t::Join( const Type_t* other )
	{
		uint32_t unJoinable = TYPE_FUNCTION | TYPE_ARRAY | TYPE_TABLE;

		// Unjoinable types? This happens if any of unJoinable is in both
		// this & other
		if ( ( m_Bits & other->m_Bits ) & unJoinable )
			return false;

		// Copy extended types
		if ( other->m_Bits & TYPE_FUNCTION )
			DeepCopyFunctionType( *other, this );

		if ( other->m_Bits & TYPE_TABLE )
			DeepCopyTableType( *other, this );

		if ( other->m_Bits & TYPE_ARRAY )
			DeepCopyArrayType( *other, this );

		return true;
	}

	void _DeepCopyType( const Type_t& source, Type_t* destination )
	{
		// Copy type bits
		destination->m_Bits = source.m_Bits;

		if ( source.m_Bits & TYPE_FUNCTION )
			DeepCopyFunctionType( source, destination );

		if ( source.m_Bits & TYPE_TABLE )
			DeepCopyTableType( source, destination );

		if ( source.m_Bits & TYPE_ARRAY )
			DeepCopyArrayType( source, destination );
	}

	Type_t* DeepCopyType( const Type_t& source, Type_t* newType )
	{
		_DeepCopyType( source, newType );
		return newType;
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

	std::string TypeToString( const Type_t& type )
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
}
