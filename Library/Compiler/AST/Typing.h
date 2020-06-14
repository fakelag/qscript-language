#pragma once

namespace Compiler
{
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
		Type_t( uint32_t bits ) : m_Bits( bits )
		{
			m_ReturnType = NULL;
		}

		Type_t* AddReturnType( uint32_t bits );
		Type_t* AddArgument( const std::string& name, uint32_t bits );
		Type_t* AddProperty( const std::string& name, uint32_t bits );
		Type_t* AddIndice( uint32_t bits );

		Type_t operator|( const Type_t& other ) 		const { return Type_t( m_Bits | other.m_Bits ); }
		uint32_t operator&( const Type_t& other ) 		const { return m_Bits & other.m_Bits; }
		uint32_t operator&( uint32_t bits ) 			const { return m_Bits & bits; }
		bool operator==( const Type_t& other ) 			const { return m_Bits == other.m_Bits; }

		bool IsUnknown()								const { return m_Bits == TYPE_UNKNOWN; }

		uint32_t 											m_Bits;
		Type_t*												m_ReturnType;	// Function returns

		std::vector< std::pair< std::string, Type_t* > >	m_ArgTypes;		// Function arguments
		std::vector< std::pair< std::string, Type_t* > >	m_PropTypes;	// Table props
		std::vector< Type_t* >								m_IndiceTypes;	// Array indices

		static std::vector< const Type_t* >					m_Types;
	};

	size_t SaveTypes( const Type_t* types );
	const Type_t* LoadTypes( size_t index );
	void FreeTypes( const Type_t* types );

	Type_t ResolveReturnType( const ListNode* funcNode, Assembler& assembler );
}
