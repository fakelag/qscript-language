#pragma once
#include "Value.h"

namespace QScript
{
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
		FORCEINLINE FunctionObject( const std::string& name, int arity, Chunk_t* chunk )
		{
			m_Type = OT_FUNCTION;
			// m_Function = function;
			m_Name = name;
			m_Arity = arity;
			m_NumUpvalues = 0;
			m_Chunk = chunk;
		}

		~FunctionObject()
		{
		}

		FORCEINLINE void Rename( const std::string& newName ) 		{ m_Name = newName; }
		FORCEINLINE const std::string& GetName() 					const { return m_Name; }
		FORCEINLINE int NumArgs() 									const { return m_Arity; }
		FORCEINLINE int NumUpvalues() 								const { return m_NumUpvalues; }
		FORCEINLINE Chunk_t* GetChunk() 							const { return m_Chunk; }

		FORCEINLINE void SetUpvalues( int numUpvalues ) { ++m_NumUpvalues; }

	private:
		std::string				m_Name;
		int						m_Arity;
		int 					m_NumUpvalues;
		Chunk_t*				m_Chunk;
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
			m_Closed = MAKE_NULL;
		}

		~UpvalueObject()
		{
		}

		FORCEINLINE Value* GetValue() { return m_Slot; }
		FORCEINLINE void SetNext( UpvalueObject* next ) { m_Next = next; }
		FORCEINLINE UpvalueObject* GetNext() { return m_Next; }
		FORCEINLINE void Close() { m_Closed = *m_Slot; m_Slot = &m_Closed; }

	private:
		Value*				m_Slot;
		Value				m_Closed;

		// m_Next: Used ONLY by the VM to track open closures
		UpvalueObject*		m_Next;
	};

	class ClosureObject : public Object
	{
	public:
		FORCEINLINE ClosureObject( const FunctionObject* function )
		{
			m_Type = OT_CLOSURE;
			m_Fn = function;
		}

		~ClosureObject()
		{
		}

		FORCEINLINE	const FunctionObject* GetFunction() const { return m_Fn; }
		FORCEINLINE	std::vector< UpvalueObject* >& GetUpvalues() { return m_Upvalues; }

	private:
		const FunctionObject* 			m_Fn;
		std::vector< UpvalueObject* >	m_Upvalues;
	};
}
