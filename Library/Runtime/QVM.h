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

	VM_t( const QScript::FunctionObject* function )
	{
		Init( function );
	}

	FORCEINLINE void Push( QScript::Value value )
	{
		if ( m_StackTop - m_Stack == m_StackCapacity )
		{
			// Reallocate more stack space
			int newCapacity = m_StackCapacity * 2;
			QScript::Value* newStack = QS_NEW QScript::Value[ newCapacity ];

			// Copy previous contents to new stack
			std::memcpy( newStack, m_Stack, ( m_StackTop - m_Stack ) * sizeof( QScript::Value ) );

			// Free previous stack space
			delete[] m_Stack;

			// Relocate call frame stack pointers
			for ( auto& frame : m_Frames )
			{
				auto stackIndex = ( uint32_t ) ( frame.m_Base - m_Stack );
				frame.m_Base = &newStack[ stackIndex ];
			}

			m_StackTop = newStack + m_StackCapacity;
			m_Stack = newStack;
			m_StackCapacity = newCapacity;
		}

		*m_StackTop = value;
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

	void Init( const QScript::FunctionObject* function );
	void Call( Frame_t* frame, uint8_t numArgs, QScript::Value& target );
	void AddObject( QScript::Object* object );

	uint8_t* OpenUpvalues( QScript::ClosureObject* closure, Frame_t* frame, uint8_t* ip );
	void CloseUpvalues( QScript::Value* last );
	void ResolveImports();

	void CreateNative( const std::string name, QScript::NativeFn native );
	void Release();

	void MarkReachable();
	void MarkObject( QScript::Object* object );
	void Recycle();

	// Call frames
	QScript::ClosureObject*								m_Main;
	std::vector< Frame_t >								m_Frames;

	// Keep track of allocated objects in the VM
	std::vector< QScript::Object* >						m_Objects;

	// Global scope
	std::unordered_map< std::string, QScript::Value >	m_Globals;

	// Stack
	QScript::Value*										m_StackTop;
	QScript::Value* 									m_Stack;
	int													m_StackCapacity;

	// Upvalues in use (not closed over)
	QScript::UpvalueObject*								m_LivingUpvalues;

	// Number of allocations until garbage collection
	int 												m_ObjectsToNextGC;
};
