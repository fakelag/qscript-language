#pragma once

struct VM_t
{
	VM_t( const QScript::Chunk_t* chunk )
	{
		m_Chunk = chunk;
		m_IP = &chunk->m_Code[ 0 ];

		m_StackTop = &m_Stack[ 0 ];
	}

	void Push( QScript::Value value )
	{
		// TODO: This is a copy operation for now
		m_StackTop->From( value );
		++m_StackTop;
	}

	QScript::Value Pop()
	{
		--m_StackTop;
		return *m_StackTop;
	}

	const QScript::Chunk_t* 			m_Chunk;
	const uint8_t*						m_IP;

	// Keep track of allocated objects in the VM
	std::vector< QScript::Object* >		m_Objects;

	QScript::Value*						m_StackTop;
	QScript::Value 						m_Stack[ 256 ];
};
