#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Instructions.h"
#include "QVM.h"
#include "../Compiler/Compiler.h"

#include "Natives.h"

#if !defined(QVM_DEBUG) && defined(_OSX)
#define _INTERP_JMP_PREFIX( opcode ) &&code_##opcode
#define INTERP_JMPTABLE static void* opcodeTable[] = { QS_OPCODES( _INTERP_JMP_PREFIX ) };
#define INTERP_SWITCH( inst ) INTERP_DISPATCH;
#define INTERP_OPCODE( opcode ) code_##opcode
#define INTERP_DISPATCH goto *opcodeTable[inst = (QScript::OpCode) READ_BYTE( vm )]
#else
#define INTERP_JMPTABLE ((void)0)
#define INTERP_SWITCH( inst ) switch ( inst )
#define INTERP_OPCODE( opcode ) case QScript::OpCode::opcode
#define INTERP_DISPATCH break
#endif

#define READ_BYTE( ) (*ip++)
#define READ_LONG( out ) uint32_t out; { \
	auto a = READ_BYTE( ); \
	auto b = READ_BYTE( ); \
	auto c = READ_BYTE( ); \
	auto d = READ_BYTE( ); \
	out = DECODE_LONG( a, b, c, d ); \
}

#define READ_CONST_SHORT() (frame->m_Function->m_Chunk->m_Constants[ READ_BYTE() ])
#define READ_CONST_LONG( constant ) QScript::Value constant; { \
	READ_LONG( cnstIndex ); \
	constant.From( frame->m_Function->m_Chunk->m_Constants[ cnstIndex ] ); \
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

		if ( frame && Compiler::FindDebugSymbol( *frame->m_Function->m_Chunk, frame->m_IP - &frame->m_Function->m_Chunk->m_Code[ 0 ], &debug ) )
		{
			token = debug.m_Token;
			lineNr = debug.m_Line;
			colNr = debug.m_Column;
		}

		throw RuntimeException( id, desc, lineNr, colNr, token );
	}

	QScript::Value Run( VM_t& vm )
	{
		Frame_t* frame = &vm.m_Frames.back();
		uint8_t* ip = frame->m_IP;

#ifdef QVM_DEBUG
		const uint8_t* runTill = NULL;
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
					std::cout << "Action (s/r/ds/dc/dca/dcnst/dg/ip/help/q): ";
					std::getline( std::cin, input );

					if ( input == "r" )
					{
						runTill = ( const uint8_t* ) -1;
						break;
					}
					else if ( input == "ds" )
					{
						Compiler::DumpStack( vm );
						continue;
					}
					else if ( input == "dc" )
					{
						Compiler::DisassembleChunk( *frame->m_Function->m_Chunk, frame->m_Function->m_Name,
							( unsigned int ) ( ip - ( uint8_t* ) &frame->m_Function->m_Chunk->m_Code[ 0 ] ) );
						continue;
					}
					else if ( input == "dca" )
					{
						Compiler::DisassembleChunk( *frame->m_Function->m_Chunk, frame->m_Function->m_Name,
							( unsigned int ) ( ip - ( uint8_t* ) &frame->m_Function->m_Chunk->m_Code[ 0 ] ) );

						for ( auto constant : frame->m_Function->m_Chunk->m_Constants )
						{
							if ( !IS_FUNCTION( constant ) )
								continue;

							auto function = AS_FUNCTION( constant )->GetProperties();
							Compiler::DisassembleChunk( *function->m_Chunk, function->m_Name );
						}
						continue;
					}
					else if ( input == "dcnst" )
					{
						Compiler::DumpConstants( *frame->m_Function->m_Chunk );
						continue;
					}
					else if ( input == "dg" )
					{
						Compiler::DumpGlobals( vm );
						continue;
					}
					else if ( input.substr( 0, 2 ) == "s " )
					{
						runTill = ( &frame->m_Function->m_Chunk->m_Code[ 0 ] ) + std::atoi( input.substr( 2 ).c_str() ) - 1;
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
						std::cout << "IP: " << ( ip - &frame->m_Function->m_Chunk->m_Code[ 0 ] ) << std::endl;
					}
					else if ( input == "q" )
						return true;
					else
						std::cout << "Invalid action: " << input << std::endl;
				}
			}
#endif

			uint8_t inst = 0;
			INTERP_SWITCH( inst )
			{
			INTERP_OPCODE( OP_LOAD_CONSTANT_SHORT ): vm.Push( READ_CONST_SHORT() ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LOAD_CONSTANT_LONG ):
			{
				READ_CONST_LONG( constant );
				vm.Push( constant );
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
			INTERP_OPCODE( OP_LOAD_GLOBAL_SHORT ):
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
			INTERP_OPCODE( OP_SET_GLOBAL_SHORT ):
			{
				auto constant = READ_CONST_SHORT();
				vm.m_Globals[ AS_STRING( constant )->GetString() ].From( vm.Peek( 0 ) );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_SET_GLOBAL_LONG ):
			{
				READ_CONST_LONG( constant );
				vm.m_Globals[ AS_STRING( constant )->GetString() ].From( vm.Peek( 0 ) );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_LOAD_LOCAL_SHORT ): vm.Push( frame->m_Base[ READ_BYTE() ] ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LOAD_LOCAL_LONG ):
			{
				READ_LONG( offset );
				vm.Push( frame->m_Base[ offset ] );
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_SET_LOCAL_SHORT ): frame->m_Base[ READ_BYTE() ].From( vm.Peek( 0 ) ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_SET_LOCAL_LONG ):
			{
				READ_LONG( offset );
				frame->m_Base[ offset ].From( vm.Peek( 0 ) );
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
				if ( ( bool ) ( vm.Peek( 0 ) ) == false )
					ip += offset;
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_JUMP_IF_ZERO_LONG ):
			{
				READ_LONG( offset );
				if ( ( bool ) ( vm.Peek( 0 ) ) == false )
					ip += offset;
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_CALL ):
			{
				uint8_t numArgs = READ_BYTE();

				frame->m_IP = ip;
				vm.Call( frame, numArgs, vm.Peek( numArgs ) );

				frame = &vm.m_Frames.back();
				ip = frame->m_IP;
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

				frame = &vm.m_Frames.back();
				ip = frame->m_IP;
				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_NOT ):
			{
				auto value = vm.Pop();
				if ( !IS_BOOL( value ) )
					QVM::RuntimeError( frame, "rt_invalid_operand_type", "Not operand must be of boolean type" );

				vm.Push( MAKE_BOOL( !( bool )( value ) ) );
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
			INTERP_OPCODE( OP_ADD ):
			{
				auto b = vm.Pop();
				auto a = vm.Pop();

				if ( IS_STRING( a ) || IS_STRING( b ) )
				{
					auto stringA = a.ToString();
					auto stringB = b.ToString();

					vm.Push( MAKE_STRING( stringA + stringB ) );
				}
				else if ( IS_NUMBER( a ) && IS_NUMBER( b ) )
				{
					vm.Push( MAKE_NUMBER( AS_NUMBER( a ) + AS_NUMBER( b ) ) );
				}
				else
				{
					QVM::RuntimeError( frame, "rt_invalid_operand_type", "Operands of \"+\" operation must be numbers or strings" );
				}

				INTERP_DISPATCH;
			}
			INTERP_OPCODE( OP_SUB ): BINARY_OP( -, IS_NUMBER ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_MUL ): BINARY_OP( *, IS_NUMBER ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_DIV ): BINARY_OP( /, IS_NUMBER ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_EQUALS ): BINARY_OP( ==, IS_ANY ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_NOT_EQUALS ): BINARY_OP( !=, IS_ANY ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_GREATERTHAN ): BINARY_OP( >, IS_ANY ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LESSTHAN ): BINARY_OP( <, IS_ANY ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_LESSTHAN_OR_EQUAL ): BINARY_OP( <=, IS_ANY ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_GREATERTHAN_OR_EQUAL ): BINARY_OP( >=, IS_ANY ); INTERP_DISPATCH;
			INTERP_OPCODE( OP_POP ): vm.Pop(); INTERP_DISPATCH;
			INTERP_OPCODE( OP_PRINT ): std::cout << (vm.Pop().ToString()) << std::endl; INTERP_DISPATCH;
			INTERP_OPCODE( OP_RETURN ):
			{
				auto returnValue = vm.Pop();

				if ( vm.m_Frames.size() == 1 )
					return returnValue;

				vm.m_StackTop = frame->m_Base;

				vm.m_Frames.pop_back();

				vm.Push( returnValue );
				frame = &vm.m_Frames.back();
				ip = frame->m_IP;
				INTERP_DISPATCH;
			}
			// default:
			// 	QVM::RuntimeError( frame, "rt_unknown_opcode", "Unknown opcode: " + std::to_string( inst ) );
			}
		}

		return MAKE_NULL;
	}

	QScript::StringObject* AllocateString( const std::string& string )
	{
		auto stringObject = new QScript::StringObject( string );
		VirtualMachine->m_Objects.push_back( ( QScript::Object* ) stringObject );
		return stringObject;
	}

	QScript::FunctionObject* AllocateFunction( const std::string& name, int arity )
	{
		auto functionObject = new QScript::FunctionObject( new QScript::Function_t( name, arity ) );
		VirtualMachine->m_Objects.push_back( ( QScript::Object* ) functionObject );
		return functionObject;
	}

	QScript::NativeFunctionObject* AllocateNative( void* nativeFn )
	{
		auto nativeObject = new QScript::NativeFunctionObject( ( QScript::NativeFn ) nativeFn );
		VirtualMachine->m_Objects.push_back( ( QScript::Object* ) nativeObject );
		return nativeObject;
	}
}

void VM_t::Init( const QScript::Function_t* function )
{
	m_Objects.clear();
	m_Globals.clear();

	m_Stack = new QScript::Value[ s_InitStackSize ];
	m_StackCapacity = s_InitStackSize;
	m_StackTop = &m_Stack[ 0 ];

	// Create initial call frame
	m_Frames.emplace_back( function, m_Stack, &function->m_Chunk->m_Code[ 0 ] );

	// Push main function to stack slot 0. This is directly allocated, so
	// the VM garbage collection won't ever release it
	Push( QScript::Value( new QScript::FunctionObject( function ) ) );
}

void VM_t::Call( Frame_t* frame, uint8_t numArgs, QScript::Value& target )
{
	if ( !IS_OBJECT( target ) )
		QVM::RuntimeError( frame, "rt_invalid_call_target", "Call value was not object type" );

	switch ( AS_OBJECT( target )->m_Type )
	{
	case QScript::ObjectType::OT_FUNCTION:
	{
		auto function = AS_FUNCTION( target )->GetProperties();
		m_Frames.emplace_back( function, m_StackTop - numArgs - 1, &function->m_Chunk->m_Code[ 0 ] );
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

void VM_t::ResolveImports()
{
	// Add a native for testing
	CreateNative( "clock", Native::clock );
}

void VM_t::CreateNative( const std::string name, QScript::NativeFn native )
{
	auto global = std::pair<std::string, QScript::Object*>( name, QVM::AllocateNative( ( void* ) native ) );
	m_Globals.insert( global );
}

void QScript::Interpret( const Function_t& function, Value* out )
{
	VM_t vm( &function );

	// Setup allocators
	QVM::VirtualMachine = &vm;
	QScript::Object::AllocateString = &QVM::AllocateString;
	QScript::Object::AllocateFunction = &QVM::AllocateFunction;
	QScript::Object::AllocateNative = &QVM::AllocateNative;

	vm.ResolveImports();

	auto exitCode = QVM::Run( vm );

	if ( out )
		out->From( exitCode );

	// Clear allocated objects
	vm.Release( out );

	// Clear allocators
	QScript::Object::AllocateString = NULL;
	QScript::Object::AllocateFunction = NULL;
	QScript::Object::AllocateNative = NULL;
}

void QScript::Interpret( VM_t& vm, Value* out )
{
	// Setup allocators
	QVM::VirtualMachine = &vm;
	QScript::Object::AllocateString = &QVM::AllocateString;
	QScript::Object::AllocateFunction = &QVM::AllocateFunction;
	QScript::Object::AllocateNative = &QVM::AllocateNative;

	vm.ResolveImports();

	auto exitCode = QVM::Run( vm );

	if ( out )
		out->From( exitCode );

	// Clear allocators
	QScript::Object::AllocateString = NULL;
	QScript::Object::AllocateFunction = NULL;
	QScript::Object::AllocateNative = NULL;
}
