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

	const char* id()			const _NOEXCEPT { return m_Id.c_str(); }
	const char* what()			const _NOEXCEPT { return m_What.c_str(); }
	const char* describe()		const _NOEXCEPT { return ( "Generic Exception (" + m_Id + "): " + m_What ).c_str(); }
protected:
	std::string m_Id;
	std::string m_What;
};

class CompilerException : public Exception
{
public:
	CompilerException( const std::string& id, const std::string& description,
		int lineNr, int colNr, const std::string& token )
		: Exception( id, description )
	{
		m_LineNr 	= lineNr;
		m_ColNr 	= colNr;
		m_Token 	= token;
	}

	const char* describe() const _NOEXCEPT
	{
		return ( "Compiler Exception (" + m_Id + "): " + m_What + " on line "
			+ std::to_string( m_LineNr ) + " character " + std::to_string( m_ColNr )
			+ " (\"" + m_Token + "\")" ).c_str();
	}

protected:
	int 			m_LineNr;
	int 			m_ColNr;
	std::string 	m_Token;
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

	const char* describe() const _NOEXCEPT
	{
		return ( "Runtime Exception (" + m_Id + "): " + m_What + " on line "
			+ std::to_string( m_LineNr ) + " character " + std::to_string( m_ColNr )
			+ " (\"" + m_Token + "\")" ).c_str();
	}

protected:
	int 			m_LineNr;
	int 			m_ColNr;
	std::string 	m_Token;
};
