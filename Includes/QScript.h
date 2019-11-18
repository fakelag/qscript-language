#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

namespace QScript
{
	struct Chunk_t
	{
		uint8_t* 	m_Code;
	};

	Chunk_t Compile( const char* pszSource );
}
