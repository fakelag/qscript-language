#pragma once

namespace Compiler
{
	struct Type_t;
	using NamedType_t = std::pair< std::string, Type_t* >;

	enum CompileTypeBits : uint32_t
	{
		// Primitives
		TYPE_UNKNOWN			= ( 0 << 0 ),
		TYPE_NULL				= ( 1 << 0 ),
		TYPE_NUMBER				= ( 1 << 1 ),
		TYPE_BOOL				= ( 1 << 2 ),

		// Objects
		TYPE_TABLE				= ( 1 << 3 ),
		TYPE_CLOSURE			= ( 1 << 4 ),
		TYPE_FUNCTION			= ( 1 << 5 ),
		TYPE_INSTANCE			= ( 1 << 6 ),
		TYPE_NATIVE				= ( 1 << 7 ),
		TYPE_STRING				= ( 1 << 8 ),
		TYPE_UPVALUE			= ( 1 << 9 ),
		TYPE_ARRAY				= ( 1 << 10 ),

		// No type (statements)
		TYPE_NONE				= ( 1 << 11 ),

		// Hint compiler to deduce type
		TYPE_AUTO				= ( 1 << 12 ),
	};

	struct Type_t
	{
		Type_t() : m_Bits( TYPE_NONE )
		{
			m_ReturnType = NULL;
		}

		Type_t( uint32_t bits ) : m_Bits( bits )
		{
			m_ReturnType = NULL;
		}

		Type_t( uint32_t bits, uint32_t returnBits ) : m_Bits( bits )
		{
			m_ReturnType = QS_NEW Type_t( returnBits );
		}

		bool IsUnknown()								const { return m_Bits == TYPE_UNKNOWN; }
		bool IsAuto()									const { return m_Bits == TYPE_AUTO; }
		bool HasPrimitive( uint32_t primitiveType )		const { return !!( m_Bits & primitiveType ); }
		bool IsPrimitive( uint32_t primitiveType )		const { return m_Bits == primitiveType; }

		bool IsAssignable( const Type_t* exprType ) const;
		bool DeepEquals( const Type_t* other ) const;
		bool Join( const Type_t* other );

		uint32_t 								m_Bits;
		Type_t*									m_ReturnType;	// Function returns

		std::vector< NamedType_t >				m_ArgTypes;		// Function arguments
		std::vector< NamedType_t >				m_PropTypes;	// Table props
		std::vector< Type_t* >					m_IndiceTypes;	// Array indices
	};

	//void DeepCopyType( const Type_t& source, Type_t* destination );
	Type_t* DeepCopyType( const Type_t& other, Type_t* allocated );
	void FreeTypes( const Type_t* types, const char* file, int line );

	// Type aliases
	static const Type_t TA_NULL = Type_t( TYPE_NULL );
	static const Type_t TA_UNKNOWN = Type_t( TYPE_UNKNOWN );
	static const Type_t TA_TABLE = Type_t( TYPE_TABLE );
	static const Type_t TA_ARRAY = Type_t( TYPE_ARRAY );
	static const Type_t TA_NUMBER = Type_t( TYPE_NUMBER );
}