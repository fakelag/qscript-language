#pragma once
#include "Value.h"

namespace QScript
{
	struct Function_t;

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
		std::string		m_String;
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
		FORCEINLINE NativeFunctionObject( )
		{
			m_Type = OT_NATIVE;
		}

		~NativeFunctionObject()
		{
		}

	private:
	};
}
