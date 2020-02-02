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
		return new QScript::StringObject( oldString->GetString() );
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
	case QScript::ObjectType::OT_INSTANCE:
	{
		auto oldInstance = ( ( QScript::InstanceObject* )( object ) );

		auto oldClass = oldInstance->GetClass();
		auto oldFields = oldInstance->GetFields();

		auto newClass = ( QScript::ClassObject* ) DeepCopyObject( oldClass );
		std::unordered_map< std::string, QScript::Value > newFields;

		for ( auto oldField : oldFields )
		{
			QScript::Value newField;
			DeepCopy( &newField, &oldField.second );

			newFields[ oldField.first ] = newField;
		}

		auto newInstance = QS_NEW QScript::InstanceObject( newClass );
		newInstance->GetFields() = newFields;

		return newInstance;
	}
	case QScript::ObjectType::OT_CLASS:
	{
		auto oldClass = ( ( QScript::ClassObject* )( object ) );
		auto newClass = QS_NEW QScript::ClassObject( oldClass->GetName() );
		return newClass;
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
	case QScript::OT_INSTANCE:
	{
		auto instObject = ( QScript::InstanceObject* ) object;
		FreeObject( instObject->GetClass() );

		for ( auto field : instObject->GetFields() )
		{
			if ( IS_OBJECT( field.second ) )
				FreeObject( AS_OBJECT( field.second ) );
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
