#pragma once

#include <vector>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

namespace QScript
{
	struct Chunk_t
	{
		std::vector< uint8_t > 	m_Code;
	};

	Chunk_t Compile( const char* pszSource );
	void Interpret( const Chunk_t& chunk );
}
