#pragma once
#include "Value.h"

namespace QScript
{
	struct Function_t;
	using NativeFn = Value (*)( const Value* args, int argCount );

	class StringObject : public Object
	{
	public:
		FORCEINLINE StringObject( const std::string& string )
		{
			m_Type = OT_STRING;
			m_String = string;
		}

		~StringObject()
		{
		}

		FORCEINLINE std::string& GetString() { return m_String; }

	private:
		std::string m_String;
	};

	class FunctionObject : public Object
	{
	public:
		FORCEINLINE FunctionObject( const Function_t* function )
		{
			m_Type = OT_FUNCTION;
			m_Function = function;
		}

		~FunctionObject()
		{
		}

		FORCEINLINE	const Function_t* GetProperties() const { return m_Function; }

	private:
		const Function_t* m_Function;
	};

	class NativeFunctionObject : public Object
	{
	public:
		FORCEINLINE NativeFunctionObject( NativeFn native )
		{
			m_Type = OT_NATIVE;
			m_Native = native;
		}

		~NativeFunctionObject()
		{
		}

		FORCEINLINE NativeFn GetNative() { return m_Native; }
	private:
		NativeFn m_Native;
	};

	class UpvalueObject : public Object
	{
	public:
		FORCEINLINE UpvalueObject( Value* value )
		{
			m_Type = OT_UPVALUE;
			m_Slot = value;
			m_Next = NULL;
			m_Closed.From( MAKE_NULL );
		}

		~UpvalueObject()
		{
		}

		FORCEINLINE Value* GetValue() { return m_Slot; }
		FORCEINLINE void SetNext( UpvalueObject* next ) { m_Next = next; }
		FORCEINLINE UpvalueObject* GetNext() { return m_Next; }
		FORCEINLINE void Close() { m_Closed.From( *m_Slot ); m_Slot = &m_Closed; }
	private:
		Value*				m_Slot;
		Value				m_Closed;
		UpvalueObject*		m_Next;
	};

	class ClosureObject : public Object
	{
	public:
		FORCEINLINE ClosureObject( FunctionObject* function )
		{
			m_Type = OT_CLOSURE;
			m_Fn = function;
		}

		~ClosureObject()
		{
		}

		FORCEINLINE	FunctionObject* GetFunction() { return m_Fn; }
		FORCEINLINE	std::vector< UpvalueObject* >& GetUpvalues() { return m_Upvalues; }

	private:
		FunctionObject* 				m_Fn;
		std::vector< UpvalueObject* >	m_Upvalues;
	};
}
