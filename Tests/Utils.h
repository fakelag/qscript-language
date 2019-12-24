#pragma once

struct VM_t;

namespace TestUtils
{
	std::string GenerateSequence( int length, std::function< std::string( int iteration ) > iterFn,
		const std::string& first = "", const std::string& last = "" );

	bool CheckVM( VM_t& vm );
	bool RunVM( const std::string& code, QScript::Value* exitCode );
}
