#pragma once

namespace TestUtils
{
	std::string GenerateSequence( int length, std::function< std::string( int iteration ) > iterFn,
		const std::string& first = "", const std::string& last = "" );
}
