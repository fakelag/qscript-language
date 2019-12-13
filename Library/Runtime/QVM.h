#pragma once

struct VM_t
{
	VM_t( const QScript::Chunk_t* chunk )
	{
		m_Chunk = chunk;
		m_IP = &chunk->m_Code[ 0 ];

		m_StackTop = &m_Stack[ 0 ];
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

	const QScript::Chunk_t* 			m_Chunk;
	const uint8_t*						m_IP;

	// Keep track of allocated objects in the VM
	std::vector< QScript::Object* >		m_Objects;

	QScript::Value*						m_StackTop;
	QScript::Value 						m_Stack[ 256 ];
};
