#include "QLibPCH.h"
#include "Utils.h"

#include "../Library/Common/Chunk.h"
#include "../Library/Common/Value.h"
#include "../Library/Runtime/QVM.h"
#include "../Library/Compiler/Compiler.h"

// Dangerous copy functions
void DeepCopy( QScript::Value* target, QScript::Value* other );
QScript::Object* DeepCopyObject( QScript::Object* object )
{
	switch ( object->m_Type )
	{
	case QScript::ObjectType::OT_FUNCTION:
	{
		auto oldFunction = ( ( QScript::FunctionObject* )( object ) );
		auto function = oldFunction->GetProperties();

		auto newFunction = new QScript::Function_t( function->m_Name, function->m_Arity, function->m_Chunk );
		newFunction->m_NumUpvalues = function->m_NumUpvalues;

		return new QScript::FunctionObject( newFunction );
	}
	case QScript::ObjectType::OT_CLOSURE:
	{
		auto oldClosure = ( ( QScript::ClosureObject* )( object ) );
		auto newFunctionObject = ( QScript::FunctionObject* ) DeepCopyObject( oldClosure->GetFunction() );

		std::vector< QScript::UpvalueObject* > upvalueList;
		for ( auto upvalue : oldClosure->GetUpvalues() )
			upvalueList.push_back( ( QScript::UpvalueObject* ) DeepCopyObject( upvalue ) );

		auto newClosureObject = new QScript::ClosureObject( newFunctionObject );
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
		
		auto newUpvalueObject = new QScript::UpvalueObject( &newValue );
		newUpvalueObject->Close();

		return newUpvalueObject;
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
		target->From( MAKE_OBJECT( DeepCopyObject( AS_OBJECT( *other ) ) ) );
	}
	else
	{
		target->From( *other );
	}
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
