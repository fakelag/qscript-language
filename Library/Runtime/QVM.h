#pragma once

struct VM_t
{
	static const int s_InitStackSize = 256;

	VM_t( const QScript::Chunk_t* chunk )
	{
		Init( chunk );
	}

	void Init( const QScript::Chunk_t* chunk )
	{
		m_Chunk = chunk;
		m_IP = &chunk->m_Code[ 0 ];
		m_Objects.clear();
		m_Globals.clear();

		m_Stack = new QScript::Value[ s_InitStackSize ];
		m_StackCapacity = s_InitStackSize;
		m_StackTop = &m_Stack[ 0 ];
	}

	FORCEINLINE void Push( QScript::Value value )
	{
		if ( m_StackTop - m_Stack == m_StackCapacity )
		{
			// Reallocate more stack space
			int newCapacity = m_StackCapacity * 2;
			QScript::Value* newStack = new QScript::Value[ newCapacity ];

			// Copy previous contents to new stack
			std::memcpy( newStack, m_Stack, ( m_StackTop - m_Stack ) * sizeof( QScript::Value ) );

			// Free previous stack space
			delete[] m_Stack;

			m_StackTop = newStack + m_StackCapacity;
			m_Stack = newStack;
			m_StackCapacity = newCapacity;
		}

		m_StackTop->From( value );
		++m_StackTop;
	}

	FORCEINLINE QScript::Value Pop()
	{
		--m_StackTop;

		if ( m_StackTop < m_Stack )
			throw RuntimeException( "rt_stack_underflow", "Stack underflow", -1, -1, "" );

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

		delete[] m_Stack;
		m_StackCapacity = 0;
		m_StackTop = NULL;

		m_Objects.clear();
	}

	const QScript::Chunk_t* 							m_Chunk;
	const uint8_t*										m_IP;

	// Keep track of allocated objects in the VM
	std::vector< QScript::Object* >						m_Objects;

	// Global scope
	std::unordered_map< std::string, QScript::Value >	m_Globals;

	QScript::Value*										m_StackTop;
	QScript::Value* 									m_Stack;
	int													m_StackCapacity;
};
