#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Instructions.h"
#include "QVM.h"
#include "../Compiler/Compiler.h"

#define READ_BYTE( vm ) (*vm.m_IP++)
#define READ_LONG( vm, out ) uint32_t out; { \
	auto a = READ_BYTE( vm ); \
	auto b = READ_BYTE( vm ); \
	auto c = READ_BYTE( vm ); \
	auto d = READ_BYTE( vm ); \
	out = DECODE_LONG( a, b, c, d ); \
}

#define READ_CONST_SHORT( vm ) (vm.m_Chunk->m_Constants[ READ_BYTE( vm ) ])
#define READ_CONST_LONG( vm, constant ) QScript::Value constant; { \
	READ_LONG( vm, cnstIndex ); \
	constant.From( vm.m_Chunk->m_Constants[ cnstIndex ] ); \
}
#define BINARY_OP( op, require ) { \
	auto b = vm.Pop(); auto a = vm.Pop(); \
	if ( !require(a) || !require(b) ) \
		throw RuntimeException( "rt_invalid_operand_type", std::string( "Both operands of \"" ) + #op + "\" operation must be numbers", -1, -1, "" ); \
	vm.Push( a op b ); \
}

namespace QVM
{
	// Let allocators to access the currently running machine
	VM_t* VirtualMachine = NULL;

	QScript::Value Run( VM_t& vm )
	{
#ifdef QVM_DEBUG
		const uint8_t* runTill = NULL;
#endif

		for (;;)
		{
#ifdef QVM_DEBUG
			if ( !runTill || vm.m_IP > runTill )
			{
				std::string input;

				for( ;; )
				{
					std::cout << "Action (s/j/r/ds/dc/dcnst/dg/help/q): ";
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
						Compiler::DisassembleChunk( *vm.m_Chunk, "current chunk", ( unsigned int ) ( vm.m_IP - ( uint8_t* ) &vm.m_Chunk->m_Code[ 0 ] ) );
						continue;
					}
					else if ( input == "dcnst" )
					{
						Compiler::DumpConstants( *vm.m_Chunk );
						continue;
					}
					else if ( input == "dg" )
					{
						Compiler::DumpGlobals( vm );
						continue;
					}
					else if ( input.substr( 0, 2 ) == "j " )
					{
						vm.m_IP = ( &vm.m_Chunk->m_Code[ 0 ] ) +  std::atoi( input.substr( 2 ).c_str() ) - 1;
						runTill = NULL;
						break;
					}
					else if ( input.substr( 0, 2 ) == "s " )
					{
						runTill = ( &vm.m_Chunk->m_Code[ 0 ] ) + std::atoi( input.substr( 2 ).c_str() ) - 1;
						break;
					}
					else if ( input == "help" )
					{
						std::cout << "" << std::endl;
						break;
					}
					else if ( input == "s" )
						break;
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
			case QScript::OP_LD_SHORT: vm.Push( READ_CONST_SHORT( vm ) ); break;
			case QScript::OP_LD_LONG:
			{
				READ_CONST_LONG( vm, constant );
				vm.Push( constant );
				break;
			}
			case QScript::OP_LG_LONG:
			{
				READ_CONST_LONG( vm, constant );
				auto global = vm.m_Globals.find( AS_STRING( constant )->GetString() );

				if ( global == vm.m_Globals.end() )
				{
					throw RuntimeException( "rt_unknown_global",
						"Referring to an unknown global: \"" + AS_STRING( constant )->GetString() + "\"", -1, -1, "" );
				}

				vm.Push( global->second );
				break;
			}
			case QScript::OP_LG_SHORT:
			{
				auto constant = READ_CONST_SHORT( vm );
				auto global = vm.m_Globals.find( AS_STRING( constant )->GetString() );

				if ( global == vm.m_Globals.end() )
				{
					throw RuntimeException( "rt_unknown_global",
						"Referring to an unknown global: \"" + AS_STRING( constant )->GetString() + "\"", -1, -1, "" );
				}

				vm.Push( global->second );
				break;
			}
			case QScript::OP_SG_SHORT:
			{
				auto constant = READ_CONST_SHORT( vm );
				vm.m_Globals[ AS_STRING( constant )->GetString() ].From( vm.Peek( 0 ) );
				vm.Pop();
				break;
			}
			case QScript::OP_SG_LONG:
			{
				READ_CONST_LONG( vm, constant );
				vm.m_Globals[ AS_STRING( constant )->GetString() ].From( vm.Peek( 0 ) );
				vm.Pop();
				break;
			}
			case QScript::OP_LL_SHORT: vm.Push( vm.m_Stack[ READ_BYTE( vm ) ] ); break;
			case QScript::OP_LL_LONG:
			{
				READ_LONG( vm, offset );
				vm.Push( vm.m_Stack[ offset ] );
				break;
			}
			case QScript::OP_SL_SHORT: vm.m_Stack[ READ_BYTE( vm ) ].From( vm.Peek( 0 ) ); break;
			case QScript::OP_SL_LONG:
			{
				READ_LONG( vm, offset );
				vm.m_Stack[ offset ].From( vm.Peek( 0 ) );
				break;
			}
			case QScript::OP_JMP_SHORT: vm.m_IP += READ_BYTE( vm ); break;
			case QScript::OP_JMP_LONG:
			{
				READ_LONG( vm, offset );
				vm.m_IP += offset;
				break;
			}
			case QScript::OP_JZ_SHORT:
			{
				auto offset = READ_BYTE( vm );
				if ( ( bool ) ( vm.Peek( 0 ) ) == false )
					vm.m_IP += offset;
				break;
			}
			case QScript::OP_JZ_LONG:
			{
				READ_LONG( vm, offset );
				if ( ( bool ) ( vm.Peek( 0 ) ) == false )
					vm.m_IP += offset;
				break;
			}
			case QScript::OP_NOT:
			{
				auto value = vm.Pop();
				if ( !IS_BOOL( value ) )
					throw RuntimeException( "rt_invalid_operand_type", "Not operand must be of boolean type", -1, -1, "" );

				vm.Push( MAKE_BOOL( !( bool )( value ) ) ); break;
				break;
			}
			case QScript::OP_NEG:
			{
				auto value = vm.Pop();
				if ( !IS_NUMBER( value ) )
					throw RuntimeException( "rt_invalid_operand_type", "Negation operand must be of number type", -1, -1, "" );

				vm.Push( MAKE_NUMBER( -AS_NUMBER( value ) ) );
				break;
			}
			case QScript::OP_PNULL: vm.Push( MAKE_NULL ); break;
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
					throw RuntimeException( "rt_invalid_operand_type", "Operands of \"+\" operation must be numbers or strings", -1, -1, "" );
				}

				break;
			}
			case QScript::OP_SUB: BINARY_OP( -, IS_NUMBER ); break;
			case QScript::OP_MUL: BINARY_OP( *, IS_NUMBER ); break;
			case QScript::OP_DIV: BINARY_OP( /, IS_NUMBER ); break;
			case QScript::OP_EQ: BINARY_OP( ==, IS_ANY ); break;
			case QScript::OP_NEQ: BINARY_OP( !=, IS_ANY ); break;
			case QScript::OP_GT: BINARY_OP( >, IS_ANY ); break;
			case QScript::OP_LT: BINARY_OP( <, IS_ANY ); break;
			case QScript::OP_LTE: BINARY_OP( <=, IS_ANY ); break;
			case QScript::OP_GTE: BINARY_OP( >=, IS_ANY ); break;
			case QScript::OP_POP: vm.Pop(); break;
			case QScript::OP_PRINT: std::cout << (vm.Pop().ToString()) << std::endl; break;
			case QScript::OP_RETN:
			{
#ifdef QVM_DEBUG
				Compiler::DumpStack( vm );

				QScript::Value exitCode;
				exitCode.From( vm.m_StackTop - vm.m_Stack > 0 ? vm.Pop() : MAKE_NULL );

				std::cout << "Exit: " << Compiler::ValueToString( exitCode ) << std::endl;
				return exitCode;
#else
				QScript::Value exitCode;
				exitCode.From( vm.m_StackTop - vm.m_Stack > 0 ? vm.Pop() : MAKE_NULL );

				return exitCode;
#endif
			}
			default:
				throw RuntimeException( "rt_unknown_opcode", "Unknown opcode: " + std::to_string( inst ), -1, -1, "" );
			}
		}
	}

	QScript::StringObject* AllocateString( const std::string& string )
	{
		auto stringObject = new QScript::StringObject( string );
		VirtualMachine->m_Objects.push_back( ( QScript::Object* ) stringObject );
		return stringObject;
	}
}

void QScript::Interpret( const Chunk_t& chunk, Value* out )
{
	VM_t vm( &chunk );

	// Setup allocators
	QVM::VirtualMachine = &vm;
	QScript::Object::AllocateString = &QVM::AllocateString;

	auto exitCode = QVM::Run( vm );

	if ( out )
		out->From( exitCode );

	// Clear allocated objects
	vm.Release( out );

	// Clear allocators
	QScript::Object::AllocateString = NULL;
}

void QScript::Interpret( VM_t& vm, Value* out )
{
	// Setup allocators
	QVM::VirtualMachine = &vm;
	QScript::Object::AllocateString = &QVM::AllocateString;

	auto exitCode = QVM::Run( vm );

	if ( out )
		out->From( exitCode );

	// Clear allocators
	QScript::Object::AllocateString = NULL;
}
