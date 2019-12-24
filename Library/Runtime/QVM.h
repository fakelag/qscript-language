#pragma once

struct VM_t
{
	VM_t( const QScript::Chunk_t* chunk )
	{
		Init( chunk );
	}

	void Init( const QScript::Chunk_t* chunk )
	{
		m_Chunk = chunk;
		m_IP = &chunk->m_Code[ 0 ];
		m_StackTop = &m_Stack[ 0 ];
		m_Objects.clear();
		m_Globals.clear();
	}

	FORCEINLINE void Push( QScript::Value value )
	{
		// TODO: This is a copy operation for now
		m_StackTop->From( value );
		++m_StackTop;
	}

	FORCEINLINE QScript::Value Pop()
	{
		--m_StackTop;
		return *m_StackTop;
	}

	FORCEINLINE QScript::Value& Peek( uint32_t offset )
	{
		return *( m_StackTop + offset - 1 );
	}

	void Release( QScript::Value* exitCode )
	{
		for ( auto object : m_Objects )
		{
			if ( exitCode && IS_OBJECT( *exitCode ) && AS_OBJECT( *exitCode ) == object )
				continue;

			delete object;
		}

		m_Objects.clear();
	}

	const QScript::Chunk_t* 							m_Chunk;
	const uint8_t*										m_IP;

	// Keep track of allocated objects in the VM
	std::vector< QScript::Object* >						m_Objects;

	// Global scope
	std::unordered_map< std::string, QScript::Value >	m_Globals;

	QScript::Value*										m_StackTop;
	QScript::Value 										m_Stack[ 256 ];
};
