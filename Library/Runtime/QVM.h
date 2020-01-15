#pragma once

struct Frame_t
{
	Frame_t( QScript::ClosureObject* closure, QScript::Value* stackFrame, uint8_t* ip )
	{
		m_Closure = closure;
		m_Base = stackFrame;
		m_IP = ip;
	}

	QScript::ClosureObject*			m_Closure;
	QScript::Value*					m_Base;
	uint8_t*						m_IP;
};

struct VM_t
{
	static const int s_InitStackSize = 256;

	VM_t( const QScript::Function_t* function )
	{
		Init( function );
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

			// Relocate call frame stack pointers
			for ( auto& frame : m_Frames )
			{
				int stackIndex = frame.m_Base - m_Stack;
				frame.m_Base = &newStack[ stackIndex ];
			}

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

	FORCEINLINE QScript::Value& Peek( int offset )
	{
		return m_StackTop[ -1 - offset ];
	}

	void Init( const QScript::Function_t* function );
	void Call( Frame_t* frame, uint8_t numArgs, QScript::Value& target );
	void ResolveImports();

	void CreateNative( const std::string name, QScript::NativeFn native );

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

	// Call frames
	std::vector< Frame_t >								m_Frames;

	// Keep track of allocated objects in the VM
	std::vector< QScript::Object* >						m_Objects;

	// Global scope
	std::unordered_map< std::string, QScript::Value >	m_Globals;

	// Stack
	QScript::Value*										m_StackTop;
	QScript::Value* 									m_Stack;
	int													m_StackCapacity;
};
