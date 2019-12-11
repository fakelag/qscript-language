#pragma once

namespace QScript
{
	enum ObjectType
	{
		OT_STRING,
	};

	class StringObject;

	class Object
	{
	public:
		virtual ~Object() {};
		ObjectType m_Type;

		using StringAllocatorFn = StringObject*(*)( const std::string& string );

		static StringAllocatorFn AllocateString;
	};

	class StringObject : public Object
	{
	public:
		FORCEINLINE StringObject( const std::string& string )
		{
			m_Type = OT_STRING;
			m_String = string;
			std::cout << "StringObject created: " + string << std::endl;
		}

		~StringObject()
		{
			std::cout << "StringObject destroyed: " + m_String << std::endl;
		}

		FORCEINLINE std::string& GetString() { return m_String; }

	private:
		std::string		m_String;
	};
}
