#include "QLibPCH.h"
#include "Chunk.h"

std::string QScript::Value::ToString() const
{
	if ( IS_OBJECT( *this ) )
	{
		auto object = AS_OBJECT( *this );

		switch ( object->m_Type )
		{
		case OT_STRING: return AS_STRING( *this )->GetString();
		case OT_FUNCTION: return "<function, " + AS_FUNCTION( *this )->GetName() + ">";
		case OT_NATIVE: return "<native code>";
		case OT_CLOSURE: return "<function, " + AS_CLOSURE( *this )->GetFunction()->GetName() + ">";
		case OT_UPVALUE: return "<upvalue>";
		case OT_TABLE: return "<table, " + AS_TABLE( *this )->GetName() + ">";
		default:
		{
#ifndef QVM_DEBUG
			UNREACHABLE();
#endif
		return "<unknown>";
		}
		}
	}
	else
	{
		if ( IS_NUMBER( *this ) )
		{
			char result[ 32 ];
			snprintf( result, sizeof( result ), "%.2f", AS_NUMBER( *this ) );
			return std::string( result );
		}
		else if ( IS_BOOL( *this ) )
			return AS_BOOL( *this ) ? "True" : "False";
		else if ( IS_NULL( *this ) )
			return "[[null]]";
		else
			return "[[unknown]]";
	}
}
