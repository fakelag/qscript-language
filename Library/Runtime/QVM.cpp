#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Instructions.h"
#include "QVM.h"
#include "../Compiler/Compiler.h"

#include "../STL/NativeModule.h"

#define INTERP_INIT \
QVM::VirtualMachine = &vm; \
QScript::Object::AllocateString = &QVM::AllocateString; \
QScript::Object::AllocateFunction = &QVM::AllocateFunction; \
QScript::Object::AllocateNative = &QVM::AllocateNative; \
QScript::Object::AllocateClosure = &QVM::AllocateClosure; \
QScript::Object::AllocateUpvalue = &QVM::AllocateUpvalue; \
QScript::Object::AllocateTable = &QVM::AllocateTable; \
QScript::Object::AllocateArray = &QVM::AllocateArray;

#define INTERP_SHUTDOWN \
QScript::Object::AllocateString = NULL; \
QScript::Object::AllocateFunction = NULL; \
QScript::Object::AllocateNative = NULL; \
QScript::Object::AllocateClosure = NULL; \
QScript::Object::AllocateUpvalue = NULL; \
QScript::Object::AllocateTable = NULL; \
QScript::Object::AllocateArray = NULL;

#if !defined(QVM_DEBUG) && defined(_OSX)
#define _INTERP_JMP_PREFIX( opcode ) &&code_##opcode
#define INTERP_JMPTABLE static void* opcodeTable[] = { QS_OPCODES( _INTERP_JMP_PREFIX ) };
#define INTERP_SWITCH( inst ) INTERP_DISPATCH;
#define INTERP_OPCODE( opcode ) code_##opcode
#define INTERP_DISPATCH goto *opcodeTable[inst = ( QScript::OpCode ) READ_BYTE( )]
#define INTERP_DEFAULT ((void)0)
#else
#define INTERP_JMPTABLE ((void)0)
#ifdef QVM_DEBUG
#define INTERP_SWITCH( inst ) inst = ( QScript::OpCode ) READ_BYTE( ); \
if (traceExec) Compiler::DisassembleInstruction( *chunk, ip - &chunk->m_Code[ 0 ], false ); \
switch ( inst )
#else
#define INTERP_SWITCH( inst ) switch ( inst = ( QScript::OpCode ) READ_BYTE( ) )
#endif
#define INTERP_OPCODE( opcode ) case QScript::OpCode::opcode
#define INTERP_DISPATCH break
#ifdef QVM_DEBUG
#define INTERP_DEFAULT default: QVM::RuntimeError( frame, "rt_unknown_opcode", "Unknown opcode: " + std::to_string( inst ) );
#else
#define INTERP_DEFAULT default: UNREACHABLE();
#endif
#endif

#define READ_BYTE( ) (*ip++)
#define READ_LONG( out ) uint32_t out; { \
	auto a = READ_BYTE( ); \
	auto b = READ_BYTE( ); \
	auto c = READ_BYTE( ); \
	auto d = READ_BYTE( ); \
	out = DECODE_LONG( a, b, c, d ); \
}

#define RESTORE_FRAME( ) \
frame = &vm.m_Frames.back(); \
ip = frame->m_IP; \
function = frame->m_Closure->GetFunction(); \
chunk = function->GetChunk() \

#define READ_CONST_SHORT() (chunk->m_Constants[ READ_BYTE() ])
#define READ_CONST_LONG( constant ) QScript::Value constant; { \
	READ_LONG( cnstIndex ); \
	constant = chunk->m_Constants[ cnstIndex ]; \
}
#define BINARY_OP( op, require ) { \
	auto b = vm.Pop(); auto a = vm.Pop(); \
	if ( !require(a) || !require(b) ) \
		QVM::RuntimeError( frame, "rt_invalid_operand_type", std::string( "Both operands of \"" ) + #op + "\" operation must be numbers" ); \
	vm.Push( a op b ); \
}

namespace QVM
{
	// Let allocators to access the currently running machine
	VM_t* VirtualMachine = NULL;

	void RuntimeError( Frame_t* frame, const std::string& id, const std::string& desc )
	{
		std::string token = "<unknown>";
		int lineNr = -1;
		int colNr = -1;

		QScript::Chunk_t::Debug_t debug;

		auto function = frame->m_Closure->GetFunction();
		if ( frame && Compiler::FindDebugSymbol( *function->GetChunk(),
			( uint32_t ) ( frame->m_IP - &function->GetChunk()->m_Code[ 0 ] ), &debug ) )
		{
			token = debug.m_Token;
			lineNr = debug.m_Line;
			colNr = debug.m_Column;
		}

		throw RuntimeException( id, desc, lineNr, colNr, token );
	}

	FORCEINLINE void LoadField( VM_t& vm, Frame_t* frame, QScript::Value& name )
	{
		auto propName = AS_STRING( name )->GetString();
		auto value = vm.Peek( 0 );

		if ( !IS_TABLE( value ) )
		{
			QVM::RuntimeError( frame, "rt_invalid_instance",
				"Can not read property \"" + propName + "\" of invalid table instance \"" + value.ToString() + "\"" );
		}

		auto table = AS_TABLE( value );
		auto& props = table->GetProperties();

		auto prop = props.find( propName );

		if ( prop == props.end() )
		{
			QVM::RuntimeError( frame, "rt_unknown_property",
				"Unknown property \"" + propName + "\" of \"" + value.ToString() + "\"" );
		}

		vm.Peek( 0 ) = prop->second;
	}

	void SetField( VM_t& vm, Frame_t* frame, QScript::Value& name )
	{
		auto propName = AS_STRING( name )->GetString();
		auto value = vm.Pop();

		if ( !IS_TABLE( value ) )
		{
			QVM::RuntimeError( frame, "rt_invalid_instance",
				"Can not read property \"" + propName + "\" of invalid table instance \"" + value.ToString() + "\"" );
		}

		auto table = AS_TABLE( value );
		auto& props = table->GetProperties();

		props[ propName ] = vm.Peek( 0 );
	}

	QScript::Value Run( VM_t& vm )
	{
		Frame_t* frame = &vm.m_Frames.back();
		uint8_t* ip = frame->m_IP;

		const QScript::FunctionObject* function = frame->m_Closure->GetFunction();
		QScript::Chunk_t* chunk = function->GetChunk();

#ifdef QVM_DEBUG
		const uint8_t* runTill = NULL;
		bool traceExec = false;
#endif

		INTERP_JMPTABLE;

		for (;;)
		{
#ifdef QVM_DEBUG
			if ( !runTill || ip > runTill )
			{
				std::string input;

				for( ;; )
				{
					std::cout << "Action (s/r/ds/dc/dca/dcnst/dg/ip/t/help/q): ";
					std::getline( std::cin, input );

					if ( input == "r" )
					{
						runTill = ( const uint8_t* ) -1;
						break;
					}
					else if ( input == "t" )
					{
						traceExec = !traceExec;
						std::cout << "Tracing: " << ( traceExec ? "Enabled" : "Disabled" ) << std::endl;
						continue;
					}
					else if ( input == "ds" )
					{
						Compiler::DumpStack( vm );
						continue;
					}
					else if ( input == "dc" )
					{
						Compiler::DisassembleChunk( *function->GetChunk(), "<function, " + function->GetName() + ">",
							( unsigned int ) ( ip - ( uint8_t* ) &function->GetChunk()->m_Code[ 0 ] ) );
						continue;
					}
					else if ( input == "dca" )
					{
						auto& main = vm.m_Frames[ 0 ];

						std::function< void( const QScript::FunctionObject* function ) > visitFunction;
						visitFunction = [ &visitFunction, &ip, &vm ]( const QScript::FunctionObject* function ) -> void
						{
							auto isExecuting = vm.m_Frames.back().m_Closure->GetFunction() == function;

							Compiler::DisassembleChunk( *function->GetChunk(), "<function, " + function->GetName() + ">",
								isExecuting ? ( unsigned int ) ( ip - ( uint8_t* ) &function->GetChunk()->m_Code[ 0 ] ) : -1 );

							for ( auto constant : function->GetChunk()->m_Constants )
							{
								if ( !IS_FUNCTION( constant ) )
									continue;

								visitFunction( AS_FUNCTION( constant ) );
							}
						};

						visitFunction( main.m_Closure->GetFunction() );
						continue;
					}
					else if ( input == "dcnst" )
					{
						Compiler::DumpConstants( *function->GetChunk() );
						continue;
					}
					else if ( input == "dg" )
					{
						Compiler::DumpGlobals( vm );
						continue;
					}
					else if ( input.substr( 0, 2 ) == "s " )
					{
						runTill = ( &function->GetChunk()->m_Code[ 0 ] ) + std::atoi( input.substr( 2 ).c_str() ) - 1;
						break;
					}
					else if ( input == "help" )
					{
						std::cout << "" << std::endl;
						break;
					}
					else if ( input == "s" )
					{
						runTill = NULL;
						break;
					}
					else if ( input == "ip" )
					{
						std::cout << "IP: " << ( ip - &function->GetChunk()->m_Code[ 0 ] ) << std::endl;
					}
					else if ( input == "q" )
						return MAKE_BOOL( true );
					else
						std::cout << "Invalid action: " << input << std::endl;
				}
			}
#endif

			uint8_t inst = 0;
			INTERP_SWITCH( inst )
			{
			INTERP_OPCODE( OP_LOAD_TOP_SHORT ): vm.Push( vm.Peek( READ_BYTE() ) ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LOAD_CONSTANT_SHORT ): vm.Push( READ_CONST_SHORT() ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LOAD_CONSTANT_LONG ):
			{
				READ_CONST_LONG( constant );
				vm.Push( constant );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_LOAD_GLOBAL_SHORT ) :
			{
				auto constant = READ_CONST_SHORT();
				auto global = vm.m_Globals.find( AS_STRING( constant )->GetString() );

				if ( global == vm.m_Globals.end() )
				{
					QVM::RuntimeError( frame, "rt_unknown_global",
						"Referring to an unknown global: \"" + AS_STRING( constant )->GetString() + "\"" );
				}

				vm.Push( global->second );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_LOAD_GLOBAL_LONG ):
			{
				READ_CONST_LONG( constant );
				auto global = vm.m_Globals.find( AS_STRING( constant )->GetString() );

				if ( global == vm.m_Globals.end() )
				{
					QVM::RuntimeError( frame, "rt_unknown_global",
						"Referring to an unknown global: \"" + AS_STRING( constant )->GetString() + "\"" );
				}

				vm.Push( global->second );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_SET_GLOBAL_SHORT ):
			{
				auto constant = READ_CONST_SHORT();
				vm.m_Globals[ AS_STRING( constant )->GetString() ] = vm.Peek( 0 );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_SET_GLOBAL_LONG ):
			{
				READ_CONST_LONG( constant );
				vm.m_Globals[ AS_STRING( constant )->GetString() ] = vm.Peek( 0 );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_LOAD_PROP_STACK ):
			{
				auto key = vm.Pop();
				auto target = vm.Peek( 0 );

				if ( IS_ARRAY( target ) )
				{
					if ( !IS_NUMBER( key ) )
					{
						QVM::RuntimeError( frame, "rt_invalid_array_index", "Invalid array index \"" + target.ToString() + "\"" );
					}

					vm.Push( AS_ARRAY( target )->GetArray()[ ( int ) AS_NUMBER( key ) ] );
				}
				else
				{
					QVM::RuntimeError( frame, "rt_invalid_instance",
						"Can not read property \"" + key.ToString() + "\" of invalid target \"" + target.ToString() + "\"" );
				}

				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_SET_PROP_STACK ):
			{
				auto key = vm.Pop();
				auto target = vm.Pop();
				auto value = vm.Peek( 0 );

				if ( IS_ARRAY( target ) )
				{
					if ( !IS_NUMBER( key ) )
						QVM::RuntimeError( frame, "rt_invalid_array_index", "Invalid array index \"" + target.ToString() + "\"" );

					int keyInt = ( int ) AS_NUMBER( key );
					auto arr = AS_ARRAY( target )->GetArray();

					if ( keyInt < 0 || keyInt >= ( int ) arr.size() )
						QVM::RuntimeError( frame, "rt_invalid_array_index", "Invalid array index \"" + target.ToString() + "\", array capacity = " + std::to_string( arr.size() ) );

					arr[ keyInt ] = value;
				}
				else
				{
					QVM::RuntimeError( frame, "rt_invalid_instance",
						"Can not read property \"" + key.ToString() + "\" of invalid target \"" + target.ToString() + "\"" );
				}

				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_PUSH_ARRAY ):
			{
				auto value = vm.Pop();
				auto target = vm.Peek( 0 );

				if ( IS_ARRAY( target ) )
				{
					AS_ARRAY( target )->GetArray().push_back( value );
				}
				else
				{
					QVM::RuntimeError( frame, "rt_invalid_instance",
						"Invalid array instance\"" + target.ToString() + "\"" );
				}

				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_LOAD_PROP_SHORT ) :
			{
				auto constant = READ_CONST_SHORT();
				LoadField( vm, frame, constant );
				INTERP_DISPATCH;
			}

			INTERP_OPCODE( OP_LOAD_PROP_LONG ) :
			{
				READ_CONST_LONG( constant );
				LoadField( vm, frame, constant );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_SET_PROP_SHORT ) :
			{
				auto constant = READ_CONST_SHORT();
				SetField( vm, frame, constant );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_SET_PROP_LONG ) :
			{
				READ_CONST_LONG( constant );
				SetField( vm, frame, constant );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_LOAD_LOCAL_0 ) :
			INTERP_OPCODE( OP_LOAD_LOCAL_1 ) :
			INTERP_OPCODE( OP_LOAD_LOCAL_2 ) :
			INTERP_OPCODE( OP_LOAD_LOCAL_3 ) :
			INTERP_OPCODE( OP_LOAD_LOCAL_4 ) :
			INTERP_OPCODE( OP_LOAD_LOCAL_5 ) :
			INTERP_OPCODE( OP_LOAD_LOCAL_6 ) :
			INTERP_OPCODE( OP_LOAD_LOCAL_7 ) :
			INTERP_OPCODE( OP_LOAD_LOCAL_8 ) :
			INTERP_OPCODE( OP_LOAD_LOCAL_9 ) :
			INTERP_OPCODE( OP_LOAD_LOCAL_10 ) :
			INTERP_OPCODE( OP_LOAD_LOCAL_11 ) :
			{
				uint8_t offset = inst - QScript::OP_LOAD_LOCAL_0;
				vm.Push( frame->m_Base[ offset ] );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_LOAD_LOCAL_SHORT ): vm.Push( frame->m_Base[ READ_BYTE() ] ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LOAD_LOCAL_LONG ):
			{
				READ_LONG( offset );
				vm.Push( frame->m_Base[ offset ] );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_SET_LOCAL_0 ):
			INTERP_OPCODE( OP_SET_LOCAL_1 ):
			INTERP_OPCODE( OP_SET_LOCAL_2 ):
			INTERP_OPCODE( OP_SET_LOCAL_3 ):
			INTERP_OPCODE( OP_SET_LOCAL_4 ):
			INTERP_OPCODE( OP_SET_LOCAL_5 ):
			INTERP_OPCODE( OP_SET_LOCAL_6 ):
			INTERP_OPCODE( OP_SET_LOCAL_7 ):
			INTERP_OPCODE( OP_SET_LOCAL_8 ):
			INTERP_OPCODE( OP_SET_LOCAL_9 ):
			INTERP_OPCODE( OP_SET_LOCAL_10 ):
			INTERP_OPCODE( OP_SET_LOCAL_11 ) :
			{
				uint8_t offset = inst - QScript::OP_SET_LOCAL_0;
				frame->m_Base[ offset ] = vm.Peek( 0 );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_SET_LOCAL_SHORT ): frame->m_Base[ READ_BYTE() ] = vm.Peek( 0 ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_SET_LOCAL_LONG ):
			{
				READ_LONG( offset );
				frame->m_Base[ offset ] = vm.Peek( 0 );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_SET_UPVALUE_SHORT ):
			{
				*( frame->m_Closure->GetUpvalues()[ READ_BYTE() ]->GetValue() ) = vm.Peek( 0 );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_SET_UPVALUE_LONG ):
			{
				READ_LONG( index );
				*( frame->m_Closure->GetUpvalues()[ index ]->GetValue() ) = vm.Peek( 0 );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_LOAD_UPVALUE_SHORT ):
			{
				vm.Push( *frame->m_Closure->GetUpvalues()[ READ_BYTE() ]->GetValue() );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_LOAD_UPVALUE_LONG ):
			{
				READ_LONG( index );
				vm.Push( *frame->m_Closure->GetUpvalues()[ index ]->GetValue() );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_JUMP_BACK_SHORT ): { auto _ip = READ_BYTE(); ip -= ( _ip + 2 ); INTERP_DISPATCH; }
			INTERP_OPCODE( OP_JUMP_BACK_LONG ):
			{
				READ_LONG( offset );
				ip -= ( offset + 5 );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_JUMP_SHORT ): { auto _ip = READ_BYTE(); ip += _ip; INTERP_DISPATCH; }
			INTERP_OPCODE( OP_JUMP_LONG ):
			{
				READ_LONG( offset );
				ip += offset;
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_JUMP_IF_ZERO_SHORT ):
			{
				auto offset = READ_BYTE();
				if ( !vm.Peek( 0 ).IsTruthy() )
					ip += offset;
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_JUMP_IF_ZERO_LONG ):
			{
				READ_LONG( offset );
				if ( !vm.Peek( 0 ).IsTruthy() )
					ip += offset;
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_CALL ):
			{
				uint8_t numArgs = READ_BYTE();

				frame->m_IP = ip;
				vm.Call( frame, numArgs, vm.Peek( numArgs ) );

				RESTORE_FRAME();
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_CALL_0 ):
			INTERP_OPCODE( OP_CALL_1 ):
			INTERP_OPCODE( OP_CALL_2 ):
			INTERP_OPCODE( OP_CALL_3 ):
			INTERP_OPCODE( OP_CALL_4 ):
			INTERP_OPCODE( OP_CALL_5 ):
			INTERP_OPCODE( OP_CALL_6 ):
			INTERP_OPCODE( OP_CALL_7 ):
			{
				uint8_t numArgs = inst - QScript::OP_CALL_0;

				frame->m_IP = ip;
				vm.Call( frame, numArgs, vm.Peek( numArgs ) );

				RESTORE_FRAME();
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_CLOSURE_SHORT ):
			{
				auto closure = MAKE_CLOSURE( AS_FUNCTION( READ_CONST_SHORT() ) );
				vm.Push( closure );

				ip = vm.OpenUpvalues( AS_CLOSURE( closure ), frame, ip );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_CLOSURE_LONG ):
			{
				READ_CONST_LONG( constant );
				auto closure = MAKE_CLOSURE( AS_FUNCTION( constant ) );
				vm.Push( closure );

				ip = vm.OpenUpvalues( AS_CLOSURE( closure ), frame, ip );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_CLOSE_UPVALUE ) :
			{
				vm.CloseUpvalues( vm.m_StackTop - 1 );
				vm.Pop();
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_NOT ):
			{
				auto value = vm.Pop();
				if ( !IS_BOOL( value ) )
					QVM::RuntimeError( frame, "rt_invalid_operand_type", "Not operand must be of boolean type" );

				vm.Push( MAKE_BOOL( !value.IsTruthy() ) );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_NEGATE ):
			{
				auto value = vm.Pop();
				if ( !IS_NUMBER( value ) )
					QVM::RuntimeError( frame, "rt_invalid_operand_type", "Negation operand must be of number type" );

				vm.Push( MAKE_NUMBER( -AS_NUMBER( value ) ) );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_LOAD_NULL ): vm.Push( MAKE_NULL ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LOAD_MINUS_1 ): vm.Push( MAKE_NUMBER( -1 ) ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LOAD_0 ): vm.Push( MAKE_NUMBER( 0 ) ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LOAD_1 ): vm.Push( MAKE_NUMBER( 1 ) ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LOAD_2 ): vm.Push( MAKE_NUMBER( 2 ) ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LOAD_3 ): vm.Push( MAKE_NUMBER( 3 ) ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LOAD_4 ): vm.Push( MAKE_NUMBER( 4 ) ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LOAD_5 ): vm.Push( MAKE_NUMBER( 5 ) ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_IMPORT ):
			{
				READ_CONST_LONG( constant );

				auto module = QScript::ResolveModule( AS_STRING( constant )->GetString() );

				if ( !module )
				{
					QVM::RuntimeError( frame, "rt_unknown_module",
						"Unknown module: \"" + AS_STRING( constant )->GetString() + "\"" );
				}
				else
				{
					module->Import( &vm );
				}

				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_BIND ):
			{
				auto closure = vm.Pop();
				auto receiver = vm.Pop();

				if ( !IS_CLOSURE( closure ) )
				{
					QVM::RuntimeError( frame, "rt_invalid_bind_target",
						"Invalid bind target: \"" + closure.ToString() + "\"" );
				}

				if ( !IS_TABLE( receiver ) )
				{
					QVM::RuntimeError( frame, "rt_invalid_bind_source",
						"Invalid bind source: \"" + receiver.ToString() + "\"" );
				}

				AS_CLOSURE( closure )->Bind( AS_OBJECT( receiver ) );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_POW ) :
			{
				auto b = vm.Pop();
				auto a = vm.Pop();

				if ( !IS_NUMBER( a ) || !IS_NUMBER( b ) )
					QVM::RuntimeError( frame, "rt_invalid_operand_type", std::string( "Both operands of \"**\" operation must be numbers" ) );

				vm.Push( a.Pow( b ) );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_ADD ) :
			{
				auto b = vm.Pop();
				auto a = vm.Pop();

				if ( IS_NUMBER( a ) && IS_NUMBER( b ) )
				{
					vm.Push( MAKE_NUMBER( AS_NUMBER( a ) + AS_NUMBER( b ) ) );
				}
				else if ( IS_STRING( a ) || IS_STRING( b ) )
				{
					auto stringA = a.ToString();
					auto stringB = b.ToString();

					vm.Push( MAKE_STRING( stringA + stringB ) );
				}
				else
				{
					QVM::RuntimeError( frame, "rt_invalid_operand_type", "Operands of \"+\" operation must be numbers or strings" );
				}

				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_NOP ): INTERP_DISPATCH;
			INTERP_OPCODE( OP_SUB ): BINARY_OP( -, IS_NUMBER ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_MUL ): BINARY_OP( *, IS_NUMBER ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_DIV ): BINARY_OP( /, IS_NUMBER ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_MOD ): BINARY_OP( %, IS_NUMBER ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_EQUALS ):
			{
				auto b = vm.Pop();
				auto a = vm.Pop();

				if ( IS_STRING( a ) && IS_STRING( b ) )
					vm.Push( MAKE_BOOL( AS_STRING( a )->GetString() == AS_STRING( b )->GetString() ) );
				else
					vm.Push( a == b );

				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_NOT_EQUALS ):
			{
				auto b = vm.Pop();
				auto a = vm.Pop();

				if ( IS_STRING( a ) && IS_STRING( b ) )
					vm.Push( MAKE_BOOL( AS_STRING( a )->GetString() != AS_STRING( b )->GetString() ) );
				else
					vm.Push( a != b );

				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_GREATERTHAN ): BINARY_OP( >, IS_ANY ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LESSTHAN ): BINARY_OP( <, IS_ANY ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LESSTHAN_OR_EQUAL ): BINARY_OP( <=, IS_ANY ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_GREATERTHAN_OR_EQUAL ): BINARY_OP( >=, IS_ANY ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_POP ): vm.Pop(); INTERP_DISPATCH;
			INTERP_OPCODE( OP_RETURN ):
			{
				auto returnValue = vm.Pop();

				// Close everything from the exiting stack frame
				vm.CloseUpvalues( frame->m_Base );

				if ( vm.m_Frames.size() == 1 )
				{
#ifdef QVM_DEBUG
					std::cout << "Exit: " << returnValue.ToString() << std::endl;
#endif
					return returnValue;
				}

				vm.m_StackTop = frame->m_Base;

				vm.m_Frames.pop_back();

				vm.Push( returnValue );

				RESTORE_FRAME();
				INTERP_DISPATCH;
			}
			INTERP_DEFAULT;
			}
		}

		return MAKE_NULL;
	}

	QScript::StringObject* AllocateString( const std::string& string )
	{
		auto stringObject = QS_NEW QScript::StringObject( string );
		VirtualMachine->AddObject( ( QScript::Object* ) stringObject );
		return stringObject;
	}

	QScript::FunctionObject* AllocateFunction( const std::string& name, int arity )
	{
		assert( 0 );

		auto functionObject = QS_NEW QScript::FunctionObject( name, arity, NULL );
		VirtualMachine->AddObject( ( QScript::Object* ) functionObject );
		return functionObject;
	}

	QScript::NativeFunctionObject* AllocateNative( void* nativeFn )
	{
		auto nativeObject = QS_NEW QScript::NativeFunctionObject( ( QScript::NativeFn ) nativeFn );
		VirtualMachine->AddObject( ( QScript::Object* ) nativeObject );
		return nativeObject;
	}

	QScript::ClosureObject* AllocateClosure( QScript::FunctionObject* function )
	{
		auto closureObject = QS_NEW QScript::ClosureObject( function );
		VirtualMachine->AddObject( ( QScript::Object* ) closureObject );
		return closureObject;
	}

	QScript::UpvalueObject* AllocateUpvalue( QScript::Value* valueRef )
	{
		auto upvalueObject = QS_NEW QScript::UpvalueObject( valueRef );
		VirtualMachine->AddObject( ( QScript::Object* ) upvalueObject );
		return upvalueObject;
	}

	QScript::TableObject* AllocateTable( const std::string& name )
	{
		auto tableObject = QS_NEW QScript::TableObject( name );
		VirtualMachine->AddObject( ( QScript::Object* ) tableObject );
		return tableObject;
	}

	QScript::ArrayObject* AllocateArray( const std::string& name )
	{
		auto arrayObject = QS_NEW QScript::ArrayObject( name );
		VirtualMachine->AddObject( ( QScript::Object* ) arrayObject );
		return arrayObject;
	}
}

void VM_t::Init( const QScript::FunctionObject* mainFunction )
{
	// Make sure module system is initialized
	QScript::InitModules();

	m_Objects.clear();
	m_Globals.clear();

	// Allocate initial stack
	m_Stack = QS_NEW QScript::Value[ s_InitStackSize ];
	m_StackCapacity = s_InitStackSize;
	m_StackTop = &m_Stack[ 0 ];

	// Get GC ready
	m_ObjectsToNextGC = 32;

	// Wrap the main function in a closure
	m_Main = QS_NEW QScript::ClosureObject( mainFunction );

	// Create initial call frame
	m_Frames.emplace_back( m_Main, m_Stack, mainFunction->GetChunk()->m_Code.data() );

	// Push main function to stack slot 0. This is directly allocated, so
	// the VM garbage collection won't ever release it
	Push( MAKE_OBJECT( m_Main ) );

	// Init upvalues linked list
	m_LivingUpvalues = NULL;
}

void VM_t::Release()
{
	// Release main
	delete m_Main;
	m_Main = NULL;

	for ( auto object : m_Objects )
		delete object;

	delete[] m_Stack;
	m_StackCapacity = 0;
	m_StackTop = NULL;

	m_Objects.clear();
}

void VM_t::Call( Frame_t* frame, uint8_t numArgs, QScript::Value& target )
{
	if ( !IS_OBJECT( target ) )
		QVM::RuntimeError( frame, "rt_invalid_call_target", "Call value was not object type" );

	switch ( AS_OBJECT( target )->m_Type )
	{
	case QScript::ObjectType::OT_CLOSURE:
	{
		auto closure = AS_CLOSURE( target );
		auto function = closure->GetFunction();

		if ( function->NumArgs() != numArgs )
		{
			QVM::RuntimeError( frame, "rt_invalid_call_arity",
				"Arguments provided is different from what the callee accepts, got: + " +
				std::to_string( numArgs ) + " expected: " + std::to_string( function->NumArgs() ) );
		}

		m_StackTop[ -numArgs - 1 ] = MAKE_OBJECT( closure->GetThis() );
		m_Frames.emplace_back( closure, m_StackTop - numArgs - 1, &function->GetChunk()->m_Code[ 0 ] );
		break;
	}
	case QScript::ObjectType::OT_NATIVE:
	{
		auto native = AS_NATIVE( target )->GetNative();
		auto returnValue = native( m_StackTop - numArgs, numArgs );
		m_StackTop -= numArgs + 1;

		Push( returnValue );
		break;
	}
	default:
		QVM::RuntimeError( frame, "rt_invalid_call_target", "Invalid call value object type" );
	}
}

void VM_t::AddObject( QScript::Object* object )
{
	m_Objects.push_back( object );

#ifdef QVM_AGGRESSIVE_GC
	if ( true )
#else
	if ( m_Objects.size() >= ( size_t ) m_ObjectsToNextGC )
#endif
	{
		// Mark object
		object->m_IsReachable = true;

		// Mark other reachable objects
		MarkReachable();

		// Sweep unreachable objects from memory
		Recycle();

		// Update next collection count
		m_ObjectsToNextGC = ( int ) m_Objects.size() * 2;
	}
}

void VM_t::MarkReachable()
{
	// Mark main
	MarkObject( m_Main );

	// Mark values sitting on the stack
	for ( QScript::Value* value = m_Stack; value < m_StackTop; ++value )
	{
		if ( !IS_OBJECT( *value ) )
			continue;

		MarkObject( AS_OBJECT( *value ) );
	}

	// Mark globals
	for ( auto& global : m_Globals )
	{
		if ( !IS_OBJECT( global.second ) )
			continue;

		MarkObject( AS_OBJECT( global.second ) );
	}

	// Mark closures of the current callstack
	for ( auto& frame : m_Frames )
		MarkObject( frame.m_Closure );

	// Mark living upvalues
	for ( auto upval = m_LivingUpvalues; upval; upval = upval->GetNext() )
		MarkObject( upval );
}

void VM_t::MarkObject( QScript::Object* object )
{
	if ( !object )
		return;

	if ( object->m_IsReachable )
		return;

	object->m_IsReachable = true;

	switch ( object->m_Type )
	{
		case QScript::OT_CLOSURE:
		{
			auto& upvalues = ( ( QScript::ClosureObject* ) object )->GetUpvalues();
			for ( auto upval : upvalues )
				MarkObject( upval );

			break;
		}
		case QScript::OT_UPVALUE:
		{
			auto value = ( ( QScript::UpvalueObject* ) object )->GetValue();

			if ( IS_OBJECT( *value ) )
				MarkObject( AS_OBJECT( *value ) );

			break;
		}
		case QScript::OT_TABLE:
		{
			auto tableObj = ( ( QScript::TableObject* ) object );

			for ( auto props : tableObj->GetProperties() )
			{
				if ( IS_OBJECT( props.second ) )
					MarkObject( AS_OBJECT( props.second ) );
			}

			break;
		}
		default:
			break;
	}
}

void VM_t::Recycle()
{
	size_t objectCount = m_Objects.size();

	for ( int i = ( int ) objectCount - 1; i >= 0; --i )
	{
		if ( m_Objects[ i ]->m_IsReachable )
			continue;

		delete m_Objects[ i ];
		m_Objects.erase( m_Objects.begin() + i );
	}

	// Reset marks on objects
	for ( auto object : m_Objects )
		object->m_IsReachable = false;

#ifdef QVM_DEBUG
	std::cout << "GC: Freed " << ( objectCount - m_Objects.size() ) << " objects" << std::endl;
#endif
}

uint8_t* VM_t::OpenUpvalues( QScript::ClosureObject* closure, Frame_t* frame, uint8_t* ip )
{
	auto function = closure->GetFunction();
	auto& upvalues = closure->GetUpvalues();

	upvalues.reserve( function->NumUpvalues() );

	for ( int i = 0; i < function->NumUpvalues(); i++ )
	{
		bool isLocal = READ_BYTE() == 1;
		READ_LONG( index );

		if ( isLocal )
		{
			QScript::Value* value = frame->m_Base + index;

			QScript::UpvalueObject* previous = NULL;
			QScript::UpvalueObject* current = m_LivingUpvalues;

			// Check if another upvalue exists that closes over the same variable
			while ( current && current->GetValue() > value )
			{
				previous = current;
				current = previous->GetNext();
			}

			if ( current && current->GetValue() == value )
			{
				// Upvalue was found on in the list, reference the same variable
				upvalues.push_back( current );
				return ip;
			}
			else
			{
				// No other function closes over this stack slot, create a new upvalue
				auto upvalue = QScript::Object::AllocateUpvalue( value );

				// Link it to the upvalue chain
				if ( !previous )
					m_LivingUpvalues = upvalue;
				else
					previous->SetNext( upvalue );

				upvalues.push_back( upvalue );
			}
		}
		else
		{
			// Refer to a parent's upvalue
			upvalues.push_back( frame->m_Closure->GetUpvalues()[ index ] );
		}
	}

	return ip;
}

void VM_t::CloseUpvalues( QScript::Value* last )
{
	// Box in every upvalue that lives at the given stack slot (or further)
	while ( m_LivingUpvalues != NULL && m_LivingUpvalues->GetValue() >= last )
	{
		m_LivingUpvalues->Close();
		m_LivingUpvalues = m_LivingUpvalues->GetNext();
	}
}

void VM_t::CreateNative( const std::string name, QScript::NativeFn native )
{
	auto global = std::pair<std::string, QScript::Value>( name, MAKE_OBJECT( QVM::AllocateNative( ( void* ) native ) ) );
	m_Globals.insert( global );
}

void QScript::Repl()
{
	static int s_ReplID = 0;

	auto chunk = QScript::AllocChunk();
	auto function = QS_NEW QScript::FunctionObject( "<repl>", 0, chunk );

	VM_t vm( function );

	INTERP_INIT;

	// Import main system module
	auto systemModule = QScript::ResolveModule( "System" );
	systemModule->Import( &vm );

	for ( ;; )
	{
		std::string source; //= "print(1?2:3);";
		std::cout << "REPL (" + std::to_string( ++s_ReplID ) << ") >";
		std::getline( std::cin, source );

		try
		{
			QScript::Config_t config( true );

			// Load globals (user defined + natives)
			for ( auto global : vm.m_Globals )
				config.m_Globals.push_back( global.first );

			INTERP_SHUTDOWN;
			auto newMain = QScript::Compile( source, config );
			INTERP_INIT;

			newMain->Rename( "<repl_" + std::to_string( s_ReplID ) + ">" );

			// Link function to main, so it gets freed at the end
			vm.m_Main->GetFunction()->GetChunk()->m_Constants.push_back( MAKE_OBJECT( newMain ) );

			// Let GC handle collecting the closure
			auto newClosure = QScript::Object::AllocateClosure( newMain );

			// Reset stack
			vm.m_StackTop = &vm.m_Stack[ 0 ];

			// New closure to stack@0
			vm.Push( MAKE_OBJECT( newClosure ) );

			// Main frame out
			vm.m_Frames.pop_back();

			// New frame in
			vm.m_Frames.emplace_back( newClosure, vm.m_Stack, &newMain->GetChunk()->m_Code[ 0 ] );

			// Run code
			QVM::Run( vm );
		}
		catch ( std::vector< CompilerException >& exceptions )
		{
			for ( auto ex : exceptions )
				std::cout << ex.describe() << std::endl;
		}
		catch ( const RuntimeException& exception )
		{
			if ( exception.id() == "rt_exit" )
				break;

			std::cout << "Exception (" << exception.id() << "): " << exception.describe() << std::endl;
		}
		catch ( const Exception& exception )
		{
			std::cout << "Exception (" << exception.id() << "): " << exception.describe() << std::endl;
		}
		catch ( const std::exception& exception )
		{
			std::cout << "Exception: " << exception.what() << std::endl;
		}
		catch ( ... )
		{
			std::cout << "Unknown exception occurred." << std::endl;
		}
	}

	vm.Release();
	QScript::FreeFunction( function );
}

void QScript::Interpret( const QScript::FunctionObject& function )
{
	VM_t vm( &function );

	INTERP_INIT;

	// Import main system module
	auto systemModule = QScript::ResolveModule( "System" );
	systemModule->Import( &vm );

	auto exitCode = QVM::Run( vm );

	// Clear allocated objects
	vm.Release();

	INTERP_SHUTDOWN;
}

void QScript::Interpret( VM_t& vm, Value* out )
{
	INTERP_INIT;

	// Import main system module
	auto systemModule = QScript::ResolveModule( "System" );
	systemModule->Import( &vm );

	auto exitCode = QVM::Run( vm );

	if ( out )
		*out = exitCode;

	INTERP_SHUTDOWN;
}
