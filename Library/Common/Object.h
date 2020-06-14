#pragma once
#include "Value.h"
#include "Typing.h"

namespace QScript
{
	using NativeFn = Value (*)( void* frame, const Value* args, int argCount );

	class StringObject : public Object
	{
	public:
		FORCEINLINE StringObject( const std::string& string )
		{
			m_Type = OT_STRING;
			m_String = string;
		}

		FORCEINLINE std::string& GetString() { return m_String; }

	private:
		std::string m_String;
	};

	class FunctionObject : public Object
	{
	public:
		struct Arg_t
		{
			std::string 		m_Name;
			Compiler::Type_t 	m_Type;
		};

		FORCEINLINE FunctionObject( const std::string& name, Chunk_t* chunk )
		{
			m_Type = OT_FUNCTION;
			m_Name = name;
			m_NumUpvalues = 0;
			m_Chunk = chunk;
		}

		FORCEINLINE void Rename( const std::string& newName ) 		{ m_Name = newName; }
		FORCEINLINE const std::string& GetName() 					const { return m_Name; }
		FORCEINLINE int NumArgs() 									const { return m_Arguments.size(); }
		FORCEINLINE int NumUpvalues() 								const { return m_NumUpvalues; }
		FORCEINLINE Chunk_t* GetChunk() 							const { return m_Chunk; }
		FORCEINLINE const std::vector< Arg_t >& GetArgs()			const { return m_Arguments; }

		FORCEINLINE void SetUpvalues( int numUpvalues ) 							{ ++m_NumUpvalues; }
		FORCEINLINE void AddArgument( const std::string& name, Compiler::Type_t type )
		{
			m_Arguments.push_back( Arg_t{ name, type } );
		}

	private:
		std::string				m_Name;
		int 					m_NumUpvalues;
		Chunk_t*				m_Chunk;
		std::vector< Arg_t >	m_Arguments;
	};

	class NativeFunctionObject : public Object
	{
	public:
		FORCEINLINE NativeFunctionObject( NativeFn native )
		{
			m_Type = OT_NATIVE;
			m_Native = native;
			m_This = NULL;
		}

		FORCEINLINE NativeFn GetNative()				{ return m_Native; }
		FORCEINLINE Object* GetThis()					{ return m_This; }
		FORCEINLINE void SetThis( Object* receiver )	{ m_This = receiver; }
	private:
		NativeFn m_Native;
		Object* m_This;
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
			m_This = this;
		}

		FORCEINLINE	const FunctionObject* GetFunction()					const { return m_Fn; }
		FORCEINLINE Object* GetThis()									const { return m_This; }
		FORCEINLINE void Bind( Object* receiver )						{ m_This = receiver; }
		FORCEINLINE	std::vector< UpvalueObject* >& GetUpvalues()		{ return m_Upvalues; }

	private:
		const FunctionObject* 			m_Fn;
		std::vector< UpvalueObject* >	m_Upvalues;
		Object*							m_This;
	};

	class TableObject : public Object
	{
	public:
		FORCEINLINE TableObject( const std::string& name )
		{
			m_Type = OT_TABLE;
			m_Name = name;
		}

		FORCEINLINE	const std::string&									GetName() const { return m_Name; }
		FORCEINLINE std::unordered_map< std::string, Value >&			GetProperties() { return m_Properties; }

	private:
		std::string									m_Name;
		std::unordered_map< std::string, Value >	m_Properties;
	};

	class ArrayObject : public Object
	{
	public:
		FORCEINLINE ArrayObject( const std::string& name )
		{
			m_Type = OT_ARRAY;
			m_Name = name;
		}

		FORCEINLINE	const std::string&								GetName() const { return m_Name; }
		FORCEINLINE std::vector< Value >&							GetArray() { return m_Array; }
		FORCEINLINE std::unordered_map< std::string, Value >&		GetMethods() { return m_Methods; }

	private:
		std::string									m_Name;
		std::vector< Value >						m_Array;
		std::unordered_map< std::string, Value >	m_Methods;
	};
}
