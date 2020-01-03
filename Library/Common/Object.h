#pragma once

namespace QScript
{
	class StringObject;
	class FunctionObject;
	struct Function_t;

	enum ObjectType
	{
		OT_STRING,
		OT_FUNCTION,
	};

	class Object
	{
	public:
		virtual ~Object() {};
		ObjectType m_Type;

		using StringAllocatorFn = StringObject*(*)( const std::string& string );
		using FunctionAllocatorFn = FunctionObject * ( *)( const std::string& name, int arity );

		static StringAllocatorFn AllocateString;
		static FunctionAllocatorFn AllocateFunction;
	};

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

		FORCEINLINE		const Function_t* GetProperties() const { return m_Function; }

	private:
		const Function_t* m_Function;
	};
}
