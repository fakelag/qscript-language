#pragma once

#include <vector>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

namespace QScript
{
	typedef double Value;

	struct Chunk_t
	{
		std::vector< uint8_t > 	m_Code;
		std::vector< Value > 	m_Constants;
	};

	Chunk_t Compile( const char* pszSource );
	void Interpret( const Chunk_t& chunk );
}
