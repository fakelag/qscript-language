#include "QLibPCH.h"
#include "Chunk.h"

std::string QScript::Value::ToString() const
{
	if ( IS_STRING( *this ) )
		return AS_STRING( *this )->GetString();
	else if ( IS_FUNCTION( *this ) )
		return AS_FUNCTION( *this )->GetProperties()->m_Name;
	else if ( IS_NATIVE( *this ) )
		return "<native code>";

	switch ( m_Type )
	{
	case VT_NUMBER:
	{
		char result[ 32 ];
		snprintf( result, sizeof( result ), "%.2f", AS_NUMBER( *this ) );
		return std::string( result );
	}
	case VT_BOOL: return AS_BOOL( *this ) ? "True" : "False";
	case VT_NULL: return "[[null]]";
	default: return "[[object]]";
	}
}
