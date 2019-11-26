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
		*m_StackTop = value;
		++m_StackTop;
	}

	QScript::Value Pop()
	{
		--m_StackTop;
		return *m_StackTop;
	}

	const QScript::Chunk_t* 			m_Chunk;
	const uint8_t*						m_IP;

	QScript::Value*						m_StackTop;
	QScript::Value 						m_Stack[ 256 ];
};
