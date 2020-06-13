#include "QLibPCH.h"
#include "Utils.h"

#include "../Library/Common/Chunk.h"
#include "../Library/Common/Value.h"
#include "../Library/Runtime/QVM.h"
#include "../Library/Compiler/Compiler.h"

// Dangerous copy functions (not recursion safe!)
void DeepCopy( QScript::Value* target, QScript::Value* other );
QScript::Object* DeepCopyObject( const QScript::Object* object )
{
	switch ( object->m_Type )
	{
	case QScript::ObjectType::OT_CLOSURE:
	{
		auto oldClosure = ( ( QScript::ClosureObject* )( object ) );
		auto newFunctionObject = ( QScript::FunctionObject* ) DeepCopyObject( oldClosure->GetFunction() );

		std::vector< QScript::UpvalueObject* > upvalueList;
		for ( auto upvalue : oldClosure->GetUpvalues() )
			upvalueList.push_back( ( QScript::UpvalueObject* ) DeepCopyObject( upvalue ) );

		auto newClosureObject = QS_NEW QScript::ClosureObject( newFunctionObject );
		newClosureObject->GetUpvalues() = upvalueList;

		return newClosureObject;
	}
	case QScript::ObjectType::OT_STRING:
	{
		auto oldString = ( ( QScript::StringObject* )( object ) );
		return QS_NEW QScript::StringObject( oldString->GetString() );
	}
	case QScript::ObjectType::OT_UPVALUE:
	{
		auto oldUpvalue = ( ( QScript::UpvalueObject* )( object ) );

		auto oldValue = oldUpvalue->GetValue();

		QScript::Value newValue;
		DeepCopy( &newValue, oldValue );

		auto newUpvalueObject = QS_NEW QScript::UpvalueObject( &newValue );
		newUpvalueObject->Close();

		return newUpvalueObject;
	}
	case QScript::ObjectType::OT_TABLE:
	{
		auto oldTable = ( ( QScript::TableObject* )( object ) );
		auto newTable = QS_NEW QScript::TableObject( oldTable->GetName() );

		for ( auto oldProp : oldTable->GetProperties() )
		{
			QScript::Value newProp;
			DeepCopy( &newProp, &oldProp.second );

			newTable->GetProperties()[ oldProp.first ] = newProp;
		}

		return newTable;
	}
	case QScript::ObjectType::OT_ARRAY:
	{
		auto oldArray = ( ( QScript::ArrayObject* )( object ) );
		auto newArray = QS_NEW QScript::ArrayObject( oldArray->GetName() );
		auto& arrayRef = oldArray->GetArray();

		for ( auto oldValue : arrayRef )
		{
			QScript::Value newValue;
			DeepCopy( &newValue, &oldValue );

			newArray->GetArray().push_back( newValue );
		}

		return newArray;
	}
	case QScript::ObjectType::OT_NATIVE:
		throw Exception( "test_objcpy_invalid_target", "Invalid target object: Native" );
	default:
		throw Exception( "test_objcpy_invalid_target", "Invalid target object: <unknown>" );
	}
}

void DeepCopy( QScript::Value* target, QScript::Value* other )
{
	if ( IS_OBJECT( *other ) )
	{
		*target = MAKE_OBJECT( DeepCopyObject( AS_OBJECT( *other ) ) );
	}
	else
	{
		*target = *other;
	}
}

// Stripped down version of releasing objects
void FreeObject( const QScript::Object* object )
{
	switch ( object->m_Type )
	{
	case QScript::OT_FUNCTION:
	{
		QScript::FreeChunk( ( ( QScript::FunctionObject* ) object )->GetChunk() );
		break;
	}
	case QScript::OT_CLOSURE:
	{
		FreeObject( ( ( QScript::ClosureObject* ) object )->GetFunction() );
		break;
	}
	case QScript::OT_TABLE:
	{
		auto tableObj = ( QScript::TableObject* ) object;

		for ( auto prop : tableObj->GetProperties() )
		{
			if ( IS_OBJECT( prop.second ) )
				FreeObject( AS_OBJECT( prop.second ) );
		}

		break;
	}
	default:
		break;
	}

	delete object;
}

bool TestUtils::FreeExitCode( QScript::Value& value )
{
	if ( IS_OBJECT( value ) )
		FreeObject( AS_OBJECT( value ) );

	return true;
}

std::string TestUtils::GenerateSequence( int length, std::function< std::string( int iteration ) > iterFn,
	const std::string& first, const std::string& last )
{
	std::string output = first;
	for ( int i = 0; i < length; ++i )
		output += iterFn( i );

	return output + last;
}

bool TestUtils::RunVM( const std::string& code, QScript::Value* exitCode )
{
	auto fn = QScript::Compile( code );

	VM_t vm( fn );

	QScript::Value vmExitCode;
	QScript::Interpret( vm, &vmExitCode );

	// Pop (function, <main>)
	vm.Pop();

	bool vmState = CheckVM( vm );

	DeepCopy( exitCode, &vmExitCode );

	vm.Release();
	QScript::FreeFunction( fn );

	return vmState;
}

bool TestUtils::CheckVM( VM_t& vm )
{
	if ( vm.m_StackTop != vm.m_Stack )
		return false;

	return true;
}
