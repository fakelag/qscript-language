#include "QLibPCH.h"
#include "Utils.h"

std::string TestUtils::GenerateSequence( int length, std::function< std::string( int iteration ) > iterFn,
	const std::string& first, const std::string& last )
{
	std::string output = first;
	for ( int i = 0; i < length; ++i )
		output += iterFn( i );

	return output + last;
}
