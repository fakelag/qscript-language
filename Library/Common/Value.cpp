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
		case OT_CLOSURE:
		{
			auto closure = AS_CLOSURE( *this );
			auto receiver = closure->GetThis();
			std::string bindString;

			if ( receiver != closure )
				bindString = ", receiver=" + MAKE_OBJECT( receiver ).ToString();

			return "<closure, " + closure->GetFunction()->GetName() + bindString + ">";
		}
		case OT_UPVALUE: return "<upvalue>";
		case OT_TABLE: return "<table, " + AS_TABLE( *this )->GetName() + ">";
		case OT_ARRAY:
		{
			auto arr = AS_ARRAY( *this );
			auto items = arr->GetArray();

			std::string itemsString;
			for ( auto item : items )
				itemsString += ", " + item.ToString();

			return "<array, " + arr->GetName() + itemsString + ">";
		}
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
