#pragma once

namespace QScript
{
	enum ObjectType
	{
		OT_STRING,
	};

	class Object
	{
	public:
		virtual ~Object() {};
		ObjectType m_Type;
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
}
