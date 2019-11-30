#pragma once

#include <vector>
#include <string>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

namespace QScript
{
	enum ValueType
	{
		VT_BOOL = 0,
		VT_NUMBER,
		VT_NULL,
	};

	struct Value
	{
		ValueType 	m_Type;
		union {
			bool	m_Bool;
			double	m_Number;
		} m_Data;
	};

	struct Chunk_t
	{
		struct Debug_t
		{
			int 				m_From;
			int 				m_To;
			int 				m_Line;
			int 				m_Column;
			std::string 		m_Token;
		};

		std::vector< uint8_t > 	m_Code;
		std::vector< Value > 	m_Constants;
		std::vector< Debug_t >	m_Debug;
	};

	Chunk_t Compile( const char* source );
	void Interpret( const Chunk_t& chunk );
}
