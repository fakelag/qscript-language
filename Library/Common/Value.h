#pragma once

#define AS_BOOL( value ) ((value).m_Data.m_Bool)
#define AS_NUMBER( value ) ((value).m_Data.m_Number)
#define AS_OBJECT( value ) ((value).m_Data.m_Object)
#define AS_STRING( value ) ((QScript::StringObject*)((value).m_Data.m_Object))
#define AS_FUNCTION( value ) ((QScript::FunctionObject*)((value).m_Data.m_Object))
#define AS_NATIVE( value ) ((QScript::NativeFunctionObject*)((value).m_Data.m_Object))

#define MAKE_NULL (QScript::Value())
#define MAKE_BOOL( value ) (QScript::Value( ((bool)(value) )))
#define MAKE_NUMBER( value ) (QScript::Value( ((double)(value) )))
#define MAKE_STRING( string ) ((QScript::Value( QScript::Object::AllocateString( string ) )))

#define IS_NULL( value ) ((value).m_Type == QScript::VT_NULL)
#define IS_BOOL( value ) ((value).m_Type == QScript::VT_BOOL)
#define IS_NUMBER( value ) ((value).m_Type == QScript::VT_NUMBER)
#define IS_OBJECT( value ) ((value).m_Type == QScript::VT_OBJECT)
#define IS_STRING( value ) ((value).IsObjectOfType<QScript::ObjectType::OT_STRING>())
#define IS_FUNCTION( value ) ((value).IsObjectOfType<QScript::ObjectType::OT_FUNCTION>())
#define IS_NATIVE( value ) ((value).IsObjectOfType<QScript::ObjectType::OT_NATIVE>())
#define IS_ANY( value ) (true)

#define ENCODE_LONG( a, index ) (( uint8_t )( ( a >> ( 8 * index ) ) & 0xFF ))
#define DECODE_LONG( a, b, c, d ) (( uint32_t ) ( a + 0x100UL * b + 0x10000UL * c + 0x1000000UL * d ))

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
		VT_NULL = 0,
		VT_NUMBER,
		VT_BOOL,
		VT_OBJECT,
	};

	enum ObjectType
	{
		OT_STRING,
		OT_FUNCTION,
		OT_NATIVE,
	};

	class StringObject;
	class FunctionObject;
	class NativeFunctionObject;

	class Object
	{
	public:
		virtual ~Object() {};
		ObjectType m_Type;

		using StringAllocatorFn = StringObject*(*)( const std::string& string );
		using FunctionAllocatorFn = FunctionObject * ( *)( const std::string& name, int arity );

		static StringAllocatorFn AllocateString;
		static FunctionAllocatorFn AllocateFunction;
	};

	// Value struct -- must be trivially copyable for stack relocations to work
	struct Value
	{
		ValueType 	m_Type;
		union {
			bool		m_Bool;
			double		m_Number;
			Object*		m_Object;
		} m_Data;

		FORCEINLINE Value() : m_Type( VT_NULL )
		{
			m_Data.m_Number = 0;
		}

		FORCEINLINE Value( bool boolean ) : m_Type( VT_BOOL )
		{
			m_Data.m_Bool = boolean;
		}

		FORCEINLINE Value( double number ) : m_Type( VT_NUMBER )
		{
			m_Data.m_Number = number;
		}

		FORCEINLINE Value( Object* object ) : m_Type( VT_OBJECT )
		{
			m_Data.m_Object = object;
		}

		FORCEINLINE Value( const Value& other )
		{
			From( other );
		}

		// No side effects, trivially copyable.
		FORCEINLINE void From( const Value& other )
		{
			m_Type = other.m_Type;
			m_Data = other.m_Data;
		}

		template<ObjectType Type>
		FORCEINLINE bool IsObjectOfType() const
		{
			return IS_OBJECT( *this ) && m_Data.m_Object->m_Type == Type;
		}

		std::string ToString() const;

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
	private:
		// Don't allow copying!
		Value operator=( const Value& other );
	};
}

#undef VALUE_CMP_OP
#undef VALUE_CMP_OP_BOOLNUM
#undef VALUE_ARIT_OP
