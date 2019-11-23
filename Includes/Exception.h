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

	const char* what() const _NOEXCEPT { return m_What.c_str(); }
protected:
	std::string m_Id;
	std::string m_What;
};
