#pragma once
#define AS_BOOL( value ) ((value).m_Data.m_Bool)
#define AS_NUMBER( value ) ((value).m_Data.m_Number)

#define MAKE_NULL ((QScript::Value){ QScript::VT_NULL, { .m_Number = 0 } })
#define MAKE_BOOL( value ) ((QScript::Value){ QScript::VT_BOOL, { .m_Bool = value } })
#define MAKE_NUMBER( value ) ((QScript::Value){ QScript::VT_NUMBER, { .m_Number = value } })

#define IS_NULL( value ) ((value).m_Type == QScript::VT_NULL)
#define IS_BOOL( value ) ((value).m_Type == QScript::VT_BOOL)
#define IS_NUMBER( value ) ((value).m_Type == QScript::VT_NUMBER)
#define IS_ANY( value ) (true)

#define VALUE_CMP_OP( op, typeCase, nullCase ) \
FORCEINLINE Value operator op ( const Value& other ) { \
	if ( m_Type != other.m_Type ) \
		return typeCase; \
	switch ( m_Type ) { \
		case VT_NULL: return nullCase; \
		case VT_BOOL: return MAKE_BOOL( AS_BOOL( *this ) op AS_BOOL( other ) ); \
		case VT_NUMBER: return MAKE_BOOL( AS_NUMBER( *this ) op AS_NUMBER( other ) ); \
		default: return MAKE_NULL; \
	} \
}

#define VALUE_CMP_OP_BOOLNUM( op, typeCase, nullCase ) \
FORCEINLINE Value operator op ( const Value& other ) { \
	if ( m_Type != other.m_Type ) \
		return typeCase; \
	switch ( m_Type ) { \
		case VT_NULL: return nullCase; \
		case VT_BOOL: return MAKE_BOOL( (AS_BOOL( *this )?1:0) op (AS_BOOL( other )?1:0) ); \
		case VT_NUMBER: return MAKE_BOOL( AS_NUMBER( *this ) op AS_NUMBER( other ) ); \
		default: return MAKE_NULL; \
	} \
}

#define VALUE_ARIT_OP( op ) \
FORCEINLINE Value operator op ( const Value& other ) { \
	switch ( m_Type ) { \
		case VT_NUMBER: return MAKE_NUMBER( AS_NUMBER( *this ) op AS_NUMBER( other ) ); \
		default: return MAKE_NULL; \
	} \
}

namespace QScript
{
	enum ValueType
	{
		VT_BOOL = 0,
		VT_NUMBER,
		VT_NULL,
	};

	struct Value
	{
		ValueType 	m_Type;
		union {
			bool	m_Bool;
			double	m_Number;
		} m_Data;

		FORCEINLINE operator bool()
		{
			if ( IS_NULL( *this ) )
				return false;

			if ( IS_BOOL( *this ) )
				return AS_BOOL( *this );

			if ( IS_NUMBER( *this ) )
				return AS_NUMBER( *this ) != 0;

			return false;
		}

		VALUE_ARIT_OP( + );
		VALUE_ARIT_OP( - );
		VALUE_ARIT_OP( / );
		VALUE_ARIT_OP( * );

		VALUE_CMP_OP( ==, MAKE_NULL, MAKE_NULL );
		VALUE_CMP_OP( !=, MAKE_NULL, MAKE_NULL );
		VALUE_CMP_OP_BOOLNUM( >, MAKE_NULL, MAKE_NULL );
		VALUE_CMP_OP_BOOLNUM( <, MAKE_NULL, MAKE_NULL );
		VALUE_CMP_OP_BOOLNUM( <=, MAKE_NULL, MAKE_NULL );
		VALUE_CMP_OP_BOOLNUM( >=, MAKE_NULL, MAKE_NULL );
	};
}

#undef VALUE_CMP_OP
#undef VALUE_CMP_OP_BOOLNUM
#undef VALUE_ARIT_OP
