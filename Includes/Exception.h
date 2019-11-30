#pragma once

#ifndef _NOEXCEPT
#define _NOEXCEPT noexcept
#endif

class Exception : public std::exception
{
public:
	Exception( const std::string& id, const std::string& description )
	{
		m_Id = id;
		m_What = description;
	}

	const char* id() const _NOEXCEPT { return m_Id.c_str(); }
	const char* what() const _NOEXCEPT { return m_What.c_str(); }
protected:
	std::string m_Id;
	std::string m_What;
};

class RuntimeException : public Exception
{
public:
	RuntimeException( const std::string& id, const std::string& description,
		int lineNr, int colNr, const std::string& token )
		: Exception( id, description )
	{
		m_LineNr 	= lineNr;
		m_ColNr 	= colNr;
		m_Token 	= token;
	}
protected:
	int 			m_LineNr;
	int 			m_ColNr;
	std::string 	m_Token;
};
