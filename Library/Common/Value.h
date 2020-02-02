#pragma once

#if not defined(QVM_DEBUG) and not defined(_DEBUG)
#define QS_NAN_BOXING
#endif

#ifdef QS_NAN_BOXING
#define NAN_SIGN_BIT				((uint64_t)1 << 63)
#define NAN_QUIET					((uint64_t)0x7FFC000000000000)

#define NAN_NULL					(1)
#define NAN_FALSE					(2)
#define NAN_TRUE					(3)
#define NAN_RESERVED_1				(4)
#define NAN_RESERVED_2				(5)
#define NAN_RESERVED_3				(6)
#define NAN_RESERVED_4				(7)

#define NULL_BITS					((uint64_t)(NAN_QUIET | NAN_NULL))
#define TRUE_BITS					((uint64_t)(NAN_QUIET | NAN_TRUE))
#define FALSE_BITS					((uint64_t)(NAN_QUIET | NAN_FALSE))

#define MAKE_OBJECT( object )		(QScript::Value(NAN_SIGN_BIT | NAN_QUIET | ( uint64_t ) ( uintptr_t ) ( object )))
#define MAKE_NULL					(QScript::Value(NULL_BITS))
#define MAKE_BOOL(value)			((bool)(value) ? (QScript::Value(TRUE_BITS)) : (QScript::Value(FALSE_BITS)))
#define MAKE_NUMBER( value )		(QScript::Value( ((double)(value) )))

#define AS_OBJECT( value )			((QScript::Object*)(( uintptr_t )( ( (value).m_Data.m_UInt64 ) & ~(NAN_SIGN_BIT | NAN_QUIET ))))
#define AS_BOOL( value )			((value).m_Data.m_UInt64 == TRUE_BITS)
#define AS_NUMBER( value )			((value).m_Data.m_Number)

#define IS_NUMBER( value )			(((value).m_Data.m_UInt64 & NAN_QUIET) != NAN_QUIET)
#define IS_OBJECT( value )			(((value).m_Data.m_UInt64 & (NAN_QUIET | NAN_SIGN_BIT)) == (NAN_QUIET | NAN_SIGN_BIT))
#define IS_NULL( value )			((value).m_Data.m_UInt64 == NULL_BITS)
#define IS_BOOL( value )			((value).IsBool())
#else
#define AS_OBJECT( value )			((value).m_Data.m_Object)
#define AS_BOOL( value )			((value).m_Data.m_Bool)
#define AS_NUMBER( value )			((value).m_Data.m_Number)

#define IS_NULL( value )			((value).m_Type == QScript::VT_NULL)
#define IS_BOOL( value )			((value).m_Type == QScript::VT_BOOL)
#define IS_NUMBER( value )			((value).m_Type == QScript::VT_NUMBER)
#define IS_OBJECT( value )			((value).m_Type == QScript::VT_OBJECT)

#define MAKE_OBJECT( object )		(QScript::Value( (( QScript::Object* ) object ) ))
#define MAKE_NULL					(QScript::Value())
#define MAKE_BOOL( value )			(QScript::Value( ((bool)(value) )))
#define MAKE_NUMBER( value )		(QScript::Value( ((double)(value) )))
#endif

#define MAKE_STRING( string )		(MAKE_OBJECT( QScript::Object::AllocateString( string ) ))
#define MAKE_CLOSURE( function )	(MAKE_OBJECT( QScript::Object::AllocateClosure( function ) ))
#define MAKE_UPVALUE( valuePtr )	(MAKE_OBJECT( QScript::Object::AllocateUpvalue( valuePtr ) ))
#define MAKE_CLASS( name )			(MAKE_OBJECT( QScript::Object::AllocateClass( name ) ))
#define MAKE_INSTANCE( classDef )	(MAKE_OBJECT( QScript::Object::AllocateInstance( classDef ) ))

#define AS_STRING( value )			((QScript::StringObject*)(AS_OBJECT(value)))
#define AS_FUNCTION( value )		((QScript::FunctionObject*)(AS_OBJECT(value)))
#define AS_NATIVE( value )			((QScript::NativeFunctionObject*)(AS_OBJECT(value)))
#define AS_CLOSURE( value )			((QScript::ClosureObject*)(AS_OBJECT(value)))
#define AS_CLASS( value )			((QScript::ClassObject*)(AS_OBJECT(value)))
#define AS_INSTANCE( value )		((QScript::InstanceObject*)(AS_OBJECT(value)))

#define IS_ANY( value ) 			(true)
#define IS_STRING( value )			((value).IsObjectOfType<QScript::ObjectType::OT_STRING>())
#define IS_FUNCTION( value )		((value).IsObjectOfType<QScript::ObjectType::OT_FUNCTION>())
#define IS_NATIVE( value )			((value).IsObjectOfType<QScript::ObjectType::OT_NATIVE>())
#define IS_CLOSURE( value )			((value).IsObjectOfType<QScript::ObjectType::OT_CLOSURE>())
#define IS_UPVALUE( value )			((value).IsObjectOfType<QScript::ObjectType::OT_UPVALUE>())
#define IS_CLASS( value )			((value).IsObjectOfType<QScript::ObjectType::OT_CLASS>())
#define IS_INSTANCE( value )		((value).IsObjectOfType<QScript::ObjectType::OT_INSTANCE>())

#define ENCODE_LONG( a, index ) (( uint8_t )( ( a >> ( 8 * index ) ) & 0xFF ))
#define DECODE_LONG( a, b, c, d ) (( uint32_t ) ( a + 0x100UL * b + 0x10000UL * c + 0x1000000UL * d ))

#ifdef QS_NAN_BOXING
#define VALUE_CMP_OP( op ) \
FORCEINLINE Value operator op ( const Value& other ) const { return MAKE_BOOL( m_Data.m_UInt64 op other.m_Data.m_UInt64 ); }

#define VALUE_ARIT_OP( op ) \
FORCEINLINE Value operator op ( const Value& other ) const { \
	return MAKE_NUMBER( AS_NUMBER( *this ) op AS_NUMBER( other ) ); \
}

#define VALUE_CMP_OP_BOOLNUM( op ) \
FORCEINLINE Value operator op ( const Value& other ) { \
	double thisNum = 0; \
	double otherNum = 0; \
	if ( IS_NUMBER( *this ) ) thisNum = AS_NUMBER( *this ); \
	else if ( IS_BOOL( *this ) ) thisNum = ( AS_BOOL( *this ) ? 1 : 0 ); \
	if ( IS_NUMBER( other ) ) otherNum = AS_NUMBER( other ); \
	else if ( IS_BOOL( other ) ) otherNum = ( AS_BOOL( other ) ? 1 : 0 ); \
	return MAKE_BOOL( thisNum op otherNum ); \
}
#else
#define VALUE_CMP_OP( op, nullCase ) \
FORCEINLINE Value operator op ( const Value& other ) { \
	if ( m_Type != other.m_Type ) \
		return nullCase; \
	switch ( m_Type ) { \
		case VT_NULL: return MAKE_NULL; \
		case VT_BOOL: return MAKE_BOOL( AS_BOOL( *this ) op AS_BOOL( other ) ); \
		case VT_NUMBER: return MAKE_BOOL( AS_NUMBER( *this ) op AS_NUMBER( other ) ); \
		case VT_OBJECT: return MAKE_BOOL( ( ( void* ) AS_OBJECT( *this ) ) op  ( ( void* ) AS_OBJECT( other ) ) ); \
		default: return nullCase; \
	} \
}

#define VALUE_CMP_OP_BOOLNUM( op ) \
FORCEINLINE Value operator op ( const Value& other ) { \
	if ( m_Type != other.m_Type ) \
		return MAKE_NULL; \
	switch ( m_Type ) { \
		case VT_NULL: return MAKE_NULL; \
		case VT_BOOL: return MAKE_BOOL( (AS_BOOL( *this )?1:0) op (AS_BOOL( other )?1:0) ); \
		case VT_NUMBER: return MAKE_BOOL( AS_NUMBER( *this ) op AS_NUMBER( other ) ); \
		default: return MAKE_NULL; \
	} \
}

#define VALUE_ARIT_OP( op ) \
FORCEINLINE Value operator op ( const Value& other ) { \
	return MAKE_NUMBER( AS_NUMBER( *this ) op AS_NUMBER( other ) ); \
}
#endif

#if defined( _MSC_VER )
#define UNREACHABLE() __assume(0)
#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#define UNREACHABLE() __builtin_unreachable()
#else
#define UNREACHABLE()
#endif

namespace QScript
{
	struct Value;
	class StringObject;
	class FunctionObject;
	class NativeFunctionObject;
	class ClosureObject;
	class UpvalueObject;
	class ClassObject;
	class InstanceObject;

	enum ValueType : char
	{
		VT_INVALID = -1,
		VT_NULL = 0,
		VT_NUMBER,
		VT_BOOL,
		VT_OBJECT,
	};

	enum ObjectType : char
	{
		OT_INVALID = -1,
		OT_CLASS,
		OT_CLOSURE,
		OT_FUNCTION,
		OT_INSTANCE,
		OT_NATIVE,
		OT_STRING,
		OT_UPVALUE,
	};

	class Object
	{
	public:
		virtual ~Object() {};

		ObjectType 		m_Type;
		bool 			m_IsReachable;

		using StringAllocatorFn = StringObject*(*)( const std::string& string );
		using FunctionAllocatorFn = FunctionObject * ( *)( const std::string& name, int arity );
		using NativeAllocatorFn = NativeFunctionObject * ( *)( void* nativeFn );
		using ClosureAllocatorFn = ClosureObject* ( *)( FunctionObject* function );
		using UpvalueAllocatorFn = UpvalueObject * ( *)( Value* valuePtr );
		using ClassAllocatorFn = ClassObject * ( *)( const std::string& name );
		using InstanceAllocatorFn = InstanceObject * ( *)( ClassObject* classDef );

		static StringAllocatorFn AllocateString;
		static FunctionAllocatorFn AllocateFunction;
		static NativeAllocatorFn AllocateNative;
		static ClosureAllocatorFn AllocateClosure;
		static UpvalueAllocatorFn AllocateUpvalue;
		static ClassAllocatorFn AllocateClass;
		static InstanceAllocatorFn AllocateInstance;
	};

	// Value struct -- must be trivially copyable for stack relocations to work
	struct Value
	{
#ifdef QS_NAN_BOXING
		union {
			uint64_t	m_UInt64;
			uint32_t	m_UInt32[ 2 ];
			double		m_Number;
		} m_Data;
#else
		ValueType 		m_Type;
		union {
			bool		m_Bool;
			double		m_Number;
			Object*		m_Object;
		} m_Data;
#endif

#ifdef QS_NAN_BOXING
		FORCEINLINE Value()
		{
			m_Data.m_Number = 0;
		}

		FORCEINLINE Value( double number )
		{
			m_Data.m_Number = number;
		}

		FORCEINLINE Value( uint64_t bits )
		{
			m_Data.m_UInt64 = bits;
		}

		FORCEINLINE bool IsBool() const { return m_Data.m_UInt64 == TRUE_BITS || m_Data.m_UInt64 == FALSE_BITS; }

		VALUE_ARIT_OP( + );
		VALUE_ARIT_OP( - );
		VALUE_ARIT_OP( / );
		VALUE_ARIT_OP( * );

		VALUE_CMP_OP( == );
		VALUE_CMP_OP( != );

		VALUE_CMP_OP_BOOLNUM( > );
		VALUE_CMP_OP_BOOLNUM( < );
		VALUE_CMP_OP_BOOLNUM( <= );
		VALUE_CMP_OP_BOOLNUM( >= );
#else
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
			m_Type = VT_NUMBER;
		}

		FORCEINLINE Value( Object* object ) : m_Type( VT_OBJECT )
		{
			m_Data.m_Object = object;
			m_Type = VT_OBJECT;
		}

		FORCEINLINE Value( const Value& other )
		{
			*this = other;
		}

		VALUE_ARIT_OP( + );
		VALUE_ARIT_OP( - );
		VALUE_ARIT_OP( / );
		VALUE_ARIT_OP( * );

		VALUE_CMP_OP( ==, MAKE_BOOL( false ) );
		VALUE_CMP_OP( !=, MAKE_BOOL( true ) );
		VALUE_CMP_OP_BOOLNUM( > );
		VALUE_CMP_OP_BOOLNUM( < );
		VALUE_CMP_OP_BOOLNUM( <= );
		VALUE_CMP_OP_BOOLNUM( >= );
#endif

	FORCEINLINE Value operator %( const Value& other ) {
		return MAKE_NUMBER( std::fmod( AS_NUMBER( *this ), AS_NUMBER( other ) ) );
	}

	FORCEINLINE Value Pow( const Value& other ) {
		return MAKE_NUMBER( std::pow( AS_NUMBER( *this ), AS_NUMBER( other ) ) );
	}

	template<ObjectType Type>
	FORCEINLINE bool IsObjectOfType() const
	{
		return IS_OBJECT( *this ) && AS_OBJECT( *this )->m_Type == Type;
	}

	FORCEINLINE bool IsTruthy() const
	{
		if ( IS_NULL( *this ) )
			return false;

		if ( IS_BOOL( *this ) )
			return AS_BOOL( *this );

		if ( IS_NUMBER( *this ) )
			return AS_NUMBER( *this ) != 0;

		return false;
	}

	std::string ToString() const;
	};
}

#undef VALUE_CMP_OP
#undef VALUE_CMP_OP_BOOLNUM
#undef VALUE_ARIT_OP
