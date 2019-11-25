#pragma once

#include <vector>
#include <string>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

namespace QScript
{
	typedef double Value;

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

	Chunk_t Compile( const char* pszSource );
	void Interpret( const Chunk_t& chunk );
}
