#include "QLibPCH.h"
#include "Array.h"
#include "../Common/Object.h"
#include "../Runtime/QVM.h"

namespace ArrayModule
{
	QScript::Value ArrayPush( void* frame, const QScript::Value* args, int argCount )
	{
		auto arr = args[ 0 ];

		if ( !IS_ARRAY( arr ) )
		{
			throw RuntimeException( "rt_native_expected",
				"Expected array object, got \"" + arr.ToString() + "\"", 0, 0, "" );
		}

		if ( argCount < 2 )
		{
			throw RuntimeException( "rt_native_argcount",
				"Expected >= 1 arguments, got " + std::to_string( argCount - 1 ), 0, 0, "" );
		}
		
		auto& arrayRef = AS_ARRAY( arr )->GetArray();

		for ( int i = 1; i < argCount; ++i )
			arrayRef.push_back( args[ i ] );

		return MAKE_NUMBER( arrayRef.size() - 1 );
	}

	QScript::Value ArrayConcat( void* frame, const QScript::Value* args, int argCount )
	{
		auto arr = args[ 0 ];

		if ( !IS_ARRAY( arr ) )
		{
			throw RuntimeException( "rt_native_expected",
				"Expected array object, got \"" + arr.ToString() + "\"", 0, 0, "" );
		}

		if ( argCount < 2 )
		{
			throw RuntimeException( "rt_native_argcount",
				"Expected >= 1 arguments, got " + std::to_string( argCount - 1 ), 0, 0, "" );
		}

		auto newArray = MAKE_ARRAY( "" );
		auto& newArrayRef = AS_ARRAY( newArray )->GetArray();
		auto& thisArrayRef = AS_ARRAY( arr )->GetArray();

		size_t totalSize = thisArrayRef.size();
		for ( int i = 1; i < argCount; ++i )
		{
			if ( !IS_ARRAY( args[ i ] ) )
			{
				throw RuntimeException( "rt_native_expected",
					"Expected array object, got \"" + args[ i ].ToString() + "\"", 0, 0, "" );
			}

			totalSize += AS_ARRAY( args[ i ] )->GetArray().size();
		}

		newArrayRef.reserve( totalSize );
		newArrayRef.insert( newArrayRef.end(), thisArrayRef.begin(), thisArrayRef.end() );

		for ( int i = 1; i < argCount; ++i )
		{
			auto& otherArrayRef = AS_ARRAY( args[ i ] )->GetArray();
			newArrayRef.insert( newArrayRef.end(), otherArrayRef.begin(), otherArrayRef.end() );
		}

		return newArray;
	}

	QScript::Value ArrayLength( void* frame, const QScript::Value* args, int argCount )
	{
		auto arr = args[ 0 ];

		if ( !IS_ARRAY( arr ) )
		{
			throw RuntimeException( "rt_native_expected",
				"Expected array object, got \"" + arr.ToString() + "\"", 0, 0, "" );
		}

		if ( argCount != 1 )
		{
			throw RuntimeException( "rt_native_argcount",
				"Expected 0 arguments, got " + std::to_string( argCount - 1 ), 0, 0, "" );
		}

		return MAKE_NUMBER( ( int ) AS_ARRAY( arr )->GetArray().size() );
	}

	QScript::Value ArrayPop( void* frame, const QScript::Value* args, int argCount )
	{
		auto arr = args[ 0 ];

		if ( !IS_ARRAY( arr ) )
		{
			throw RuntimeException( "rt_native_expected",
				"Expected array object, got \"" + arr.ToString() + "\"", 0, 0, "" );
		}

		if ( argCount != 1 )
		{
			throw RuntimeException( "rt_native_argcount",
				"Expected 0 arguments, got " + std::to_string( argCount - 1 ), 0, 0, "" );
		}

		auto& arrayRef = AS_ARRAY( arr )->GetArray();
		auto returnValue = arrayRef.back();
		
		arrayRef.pop_back();
		return returnValue;
	}

	QScript::Value ArrayFilter( void* frame, const QScript::Value* args, int argCount )
	{
		auto arr = args[ 0 ];

		if ( !IS_ARRAY( arr ) )
		{
			throw RuntimeException( "rt_native_expected",
				"Expected array object, got \"" + arr.ToString() + "\"", 0, 0, "" );
		}

		if ( argCount != 2 )
		{
			throw RuntimeException( "rt_native_argcount",
				"Expected 1 arguments, got " + std::to_string( argCount - 1 ), 0, 0, "" );
		}

		auto filterFn = args[ 1 ];
		auto& arrayRef = AS_ARRAY( arr )->GetArray();

		arrayRef.erase( std::remove_if( arrayRef.begin(), arrayRef.end(), [ &filterFn, frame ]( QScript::Value value ) {
			QVM::VirtualMachine->Push( filterFn );
			QVM::VirtualMachine->Push( value );

			QVM::VirtualMachine->Call( ( Frame_t* ) frame, 1, filterFn, true );
			QVM::Run( *QVM::VirtualMachine, false );

			return !QVM::VirtualMachine->Pop().IsTruthy();
		} ), arrayRef.end() );

		// filterFn is currently at the top of the stack, but within our call frame,
		// so it will get cleaned up when we return here

		return args[ 0 ];
	}

	QScript::Value ArrayFind( void* frame, const QScript::Value* args, int argCount )
	{
		auto arr = args[ 0 ];

		if ( !IS_ARRAY( arr ) )
		{
			throw RuntimeException( "rt_native_expected",
				"Expected array object, got \"" + arr.ToString() + "\"", 0, 0, "" );
		}

		if ( argCount != 2 )
		{
			throw RuntimeException( "rt_native_argcount",
				"Expected 1 arguments, got " + std::to_string( argCount - 1 ), 0, 0, "" );
		}

		auto findFn = args[ 1 ];
		auto& arrayRef = AS_ARRAY( arr )->GetArray();

		auto element = std::find_if( arrayRef.begin(), arrayRef.end(), [ &findFn, frame ]( QScript::Value value ) {
			QVM::VirtualMachine->Push( findFn );
			QVM::VirtualMachine->Push( value );

			QVM::VirtualMachine->Call( ( Frame_t* ) frame, 1, findFn, true );
			QVM::Run( *QVM::VirtualMachine, false );

			return !QVM::VirtualMachine->Pop().IsTruthy();
		} );

		if ( element == arrayRef.end() )
			return MAKE_NULL;

		return *element;
	}

	QScript::Value ArraySlice( void* frame, const QScript::Value* args, int argCount )
	{
		auto arr = args[ 0 ];

		if ( !IS_ARRAY( arr ) )
		{
			throw RuntimeException( "rt_native_expected",
				"Expected array object, got \"" + arr.ToString() + "\"", 0, 0, "" );
		}

		if ( argCount < 2 )
		{
			throw RuntimeException( "rt_native_argcount",
				"Expected >= 1 arguments, got " + std::to_string( argCount - 1 ), 0, 0, "" );
		}

		if ( !IS_NUMBER( args[ 1 ] ) || ( argCount > 2 && !IS_NUMBER( args[ 2 ] ) ) )
		{
			throw RuntimeException( "rt_native_expected",
				"Both arguments of slice must be numbers, got \""
				+ args[ 1 ].ToString() + ( argCount > 2 ? "\", \"" + args[ 2 ].ToString() + "\"" : "\"" ), 0, 0, "" );
		}

		auto newArray = MAKE_ARRAY( "" );
		auto& arrayRef = AS_ARRAY( arr )->GetArray();

		int startSlice = std::max( 0, ( int ) AS_NUMBER( args[ 1 ] ) );

		if ( startSlice >= ( int ) arrayRef.size() )
			return newArray;

		int endSlice = ( int ) arrayRef.size();
		
		if ( argCount > 2 )
		{
			endSlice = std::min( ( int ) arrayRef.size(), std::max( 0, ( int ) AS_NUMBER( args[ 2 ] ) ) );

			if ( endSlice < startSlice )
				return newArray;
		}

		AS_ARRAY( newArray )->GetArray() = std::vector< QScript::Value >( arrayRef.begin() + startSlice, arrayRef.begin() + endSlice );
		return newArray;
	}

	void LoadMethods( VM_t* vm )
	{
		vm->CreateArrayMethod( "push", ArrayPush );
		vm->CreateArrayMethod( "concat", ArrayConcat );
		vm->CreateArrayMethod( "length", ArrayLength );
		vm->CreateArrayMethod( "pop", ArrayPop );
		vm->CreateArrayMethod( "filter", ArrayFilter );
		vm->CreateArrayMethod( "find", ArrayFind );
		vm->CreateArrayMethod( "slice", ArraySlice );
	}
}
