#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Instructions.h"
#include "QVM.h"
#include "../Compiler/Compiler.h"

#define READ_BYTE( vm ) (*frame->m_IP++)
#define READ_LONG( vm, out ) uint32_t out; { \
	auto a = READ_BYTE( vm ); \
	auto b = READ_BYTE( vm ); \
	auto c = READ_BYTE( vm ); \
	auto d = READ_BYTE( vm ); \
	out = DECODE_LONG( a, b, c, d ); \
}

#define READ_CONST_SHORT( vm ) (frame->m_Function->m_Chunk->m_Constants[ READ_BYTE( vm ) ])
#define READ_CONST_LONG( vm, constant ) QScript::Value constant; { \
	READ_LONG( vm, cnstIndex ); \
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

#ifdef QVM_DEBUG
		const uint8_t* runTill = NULL;
#endif

		for (;;)
		{
#ifdef QVM_DEBUG
			if ( !runTill || frame->m_IP > runTill )
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
							( unsigned int ) ( frame->m_IP - ( uint8_t* ) &frame->m_Function->m_Chunk->m_Code[ 0 ] ) );
						continue;
					}
					else if ( input == "dca" )
					{
						Compiler::DisassembleChunk( *frame->m_Function->m_Chunk, frame->m_Function->m_Name,
							( unsigned int ) ( frame->m_IP - ( uint8_t* ) &frame->m_Function->m_Chunk->m_Code[ 0 ] ) );

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
						std::cout << "IP: " << ( frame->m_IP - &frame->m_Function->m_Chunk->m_Code[ 0 ] ) << std::endl;
					}
					else if ( input == "q" )
						return true;
					else
						std::cout << "Invalid action: " << input << std::endl;
				}
			}

			// Compiler::DisassembleInstruction( *vm.m_Chunk, ( int )( vm.m_IP - ( uint8_t* ) &vm.m_Chunk->m_Code[ 0 ] ) );
#endif

			uint8_t inst = READ_BYTE( vm );
			switch ( inst )
			{
			case QScript::OP_LOAD_CONSTANT_SHORT: vm.Push( READ_CONST_SHORT( vm ) ); break;
			case QScript::OP_LOAD_CONSTANT_LONG:
			{
				READ_CONST_LONG( vm, constant );
				vm.Push( constant );
				break;
			}
			case QScript::OP_LOAD_GLOBAL_LONG:
			{
				READ_CONST_LONG( vm, constant );
				auto global = vm.m_Globals.find( AS_STRING( constant )->GetString() );

				if ( global == vm.m_Globals.end() )
				{
					QVM::RuntimeError( frame, "rt_unknown_global",
						"Referring to an unknown global: \"" + AS_STRING( constant )->GetString() + "\"" );
				}

				vm.Push( global->second );
				break;
			}
			case QScript::OP_LOAD_GLOBAL_SHORT:
			{
				auto constant = READ_CONST_SHORT( vm );
				auto global = vm.m_Globals.find( AS_STRING( constant )->GetString() );

				if ( global == vm.m_Globals.end() )
				{
					QVM::RuntimeError( frame, "rt_unknown_global",
						"Referring to an unknown global: \"" + AS_STRING( constant )->GetString() + "\"" );
				}

				vm.Push( global->second );
				break;
			}
			case QScript::OP_SET_GLOBAL_SHORT:
			{
				auto constant = READ_CONST_SHORT( vm );
				vm.m_Globals[ AS_STRING( constant )->GetString() ].From( vm.Peek( 0 ) );
				break;
			}
			case QScript::OP_SET_GLOBAL_LONG:
			{
				READ_CONST_LONG( vm, constant );
				vm.m_Globals[ AS_STRING( constant )->GetString() ].From( vm.Peek( 0 ) );
				break;
			}
			case QScript::OP_LOAD_LOCAL_SHORT: vm.Push( frame->m_Base[ READ_BYTE( vm ) ] ); break;
			case QScript::OP_LOAD_LOCAL_LONG:
			{
				READ_LONG( vm, offset );
				vm.Push( frame->m_Base[ offset ] );
				break;
			}
			case QScript::OP_SET_LOCAL_SHORT: frame->m_Base[ READ_BYTE( vm ) ].From( vm.Peek( 0 ) ); break;
			case QScript::OP_SET_LOCAL_LONG:
			{
				READ_LONG( vm, offset );
				frame->m_Base[ offset ].From( vm.Peek( 0 ) );
				break;
			}
			case QScript::OP_JUMP_BACK_SHORT: frame->m_IP -= ( READ_BYTE( vm ) + 2 ); break;
			case QScript::OP_JUMP_BACK_LONG:
			{
				READ_LONG( vm, offset );
				frame->m_IP -= ( offset + 5 );
				break;
			}
			case QScript::OP_JUMP_SHORT: frame->m_IP += READ_BYTE( vm ); break;
			case QScript::OP_JUMP_LONG:
			{
				READ_LONG( vm, offset );
				frame->m_IP += offset;
				break;
			}
			case QScript::OP_JUMP_IF_ZERO_SHORT:
			{
				auto offset = READ_BYTE( vm );
				if ( ( bool ) ( vm.Peek( 0 ) ) == false )
					frame->m_IP += offset;
				break;
			}
			case QScript::OP_JUMP_IF_ZERO_LONG:
			{
				READ_LONG( vm, offset );
				if ( ( bool ) ( vm.Peek( 0 ) ) == false )
					frame->m_IP += offset;
				break;
			}
			case QScript::OP_CALL:
			{
				uint8_t numArgs = READ_BYTE( vm );
				vm.Call( frame, numArgs, vm.Peek( numArgs ) );
				frame = &vm.m_Frames.back();
				break;
			}
			case QScript::OP_NOT:
			{
				auto value = vm.Pop();
				if ( !IS_BOOL( value ) )
					QVM::RuntimeError( frame, "rt_invalid_operand_type", "Not operand must be of boolean type" );

				vm.Push( MAKE_BOOL( !( bool )( value ) ) ); break;
				break;
			}
			case QScript::OP_NEGATE:
			{
				auto value = vm.Pop();
				if ( !IS_NUMBER( value ) )
					QVM::RuntimeError( frame, "rt_invalid_operand_type", "Negation operand must be of number type" );

				vm.Push( MAKE_NUMBER( -AS_NUMBER( value ) ) );
				break;
			}
			case QScript::OP_LOAD_NULL: vm.Push( MAKE_NULL ); break;
			case QScript::OP_ADD:
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

				break;
			}
			case QScript::OP_SUB: BINARY_OP( -, IS_NUMBER ); break;
			case QScript::OP_MUL: BINARY_OP( *, IS_NUMBER ); break;
			case QScript::OP_DIV: BINARY_OP( /, IS_NUMBER ); break;
			case QScript::OP_EQUALS: BINARY_OP( ==, IS_ANY ); break;
			case QScript::OP_NOT_EQUALS: BINARY_OP( !=, IS_ANY ); break;
			case QScript::OP_GREATERTHAN: BINARY_OP( >, IS_ANY ); break;
			case QScript::OP_LESSTHAN: BINARY_OP( <, IS_ANY ); break;
			case QScript::OP_LESSTHAN_OR_EQUAL: BINARY_OP( <=, IS_ANY ); break;
			case QScript::OP_GREATERTHAN_OR_EQUAL: BINARY_OP( >=, IS_ANY ); break;
			case QScript::OP_POP: vm.Pop(); break;
			case QScript::OP_PRINT: std::cout << (vm.Pop().ToString()) << std::endl; break;
			case QScript::OP_RETURN:
			{
				auto returnValue = vm.Pop();
				vm.m_Frames.pop_back();

				if ( vm.m_Frames.size() == 0 )
					return returnValue;

				vm.m_StackTop = frame->m_Base;

				vm.Push( returnValue );
				frame = &vm.m_Frames.back();
				break;
			}
			default:
				QVM::RuntimeError( frame, "rt_unknown_opcode", "Unknown opcode: " + std::to_string( inst ) );
			}
		}
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
	default:
		QVM::RuntimeError( frame, "rt_invalid_call_target", "Invalid call value object type" );
	}
}

void QScript::Interpret( const Function_t& function, Value* out )
{
	VM_t vm( &function );

	// Setup allocators
	QVM::VirtualMachine = &vm;
	QScript::Object::AllocateString = &QVM::AllocateString;
	QScript::Object::AllocateFunction = &QVM::AllocateFunction;

	auto exitCode = QVM::Run( vm );

	if ( out )
		out->From( exitCode );

	// Clear allocated objects
	vm.Release( out );

	// Clear allocators
	QScript::Object::AllocateString = NULL;
	QScript::Object::AllocateFunction = NULL;
}

void QScript::Interpret( VM_t& vm, Value* out )
{
	// Setup allocators
	QVM::VirtualMachine = &vm;
	QScript::Object::AllocateString = &QVM::AllocateString;
	QScript::Object::AllocateFunction = &QVM::AllocateFunction;

	auto exitCode = QVM::Run( vm );

	if ( out )
		out->From( exitCode );

	// Clear allocators
	QScript::Object::AllocateString = NULL;
	QScript::Object::AllocateFunction = NULL;
}
