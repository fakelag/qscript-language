#include "QLibPCH.h"
#include "../Common/Chunk.h"

#include "Instructions.h"
#include "QVM.h"
#include "../Compiler/Compiler.h"

#define READ_BYTE( vm ) (*vm.m_IP++)
#define READ_CONST( vm ) (vm.m_Chunk->m_Constants[ READ_BYTE( vm ) ])
#define BINARY_OP( op, require ) { \
	auto b = vm.Pop(); auto a = vm.Pop(); \
	if ( !require(a) || !require(b) ) \
		throw RuntimeException( "rt_invalid_operand_type", std::string( "Both operands of \"" ) + #op + "\" operation must be numbers", -1, -1, "" ); \
	vm.Push( a op b ); \
}

bool Run( VM_t& vm )
{
#ifdef QVM_DEBUG
	bool isStepping = true;
#endif

	for (;;)
	{
#ifdef QVM_DEBUG
		if ( isStepping )
		{
			std::string input;

			for( ;; )
			{
				std::cout << "Action (s/r/ds/dc/q): ";
				std::cin >> input;

				if ( input == "r" )
				{
					isStepping = false;
					break;
				}
				else if ( input == "ds" )
				{
					Compiler::DumpStack( vm );
					continue;
				}
				else if ( input == "dc" )
				{
					Compiler::DisassembleChunk( *vm.m_Chunk, "current chunk" );
					continue;
				}
				else if ( input == "s" )
					break;
				else if ( input == "q" )
					return true;
				else
					std::cout << "Invalid action: " << input << std::endl;
			}
		}

		Compiler::DisassembleInstruction( *vm.m_Chunk, ( int )( vm.m_IP - ( uint8_t* ) &vm.m_Chunk->m_Code[ 0 ] ) );
#endif

		uint8_t inst = READ_BYTE( vm );
		switch ( inst )
		{
		case QScript::OP_LOAD: vm.Push( READ_CONST( vm ) ); break;
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
		case QScript::OP_ADD: BINARY_OP( +, IS_NUMBER ); break;
		case QScript::OP_SUB: BINARY_OP( -, IS_NUMBER ); break;
		case QScript::OP_MUL: BINARY_OP( *, IS_NUMBER ); break;
		case QScript::OP_DIV: BINARY_OP( /, IS_NUMBER ); break;
		case QScript::OP_EQ: BINARY_OP( ==, IS_ANY ); break;
		case QScript::OP_NEQ: BINARY_OP( !=, IS_ANY ); break;
		case QScript::OP_GT: BINARY_OP( >, IS_ANY ); break;
		case QScript::OP_LT: BINARY_OP( <, IS_ANY ); break;
		case QScript::OP_LTE: BINARY_OP( <=, IS_ANY );break;
		case QScript::OP_GTE: BINARY_OP( >=, IS_ANY );break;
		case QScript::OP_RETN:
		{
#ifdef QVM_DEBUG
			Compiler::DumpStack( vm );
#endif
			std::cout << "Exit: " << Compiler::ValueToString( vm.Pop() ) << std::endl;
			return true;
		}
		default:
			throw RuntimeException( "rt_unknown_opcode", "Unknown opcode: " + std::to_string( inst ), -1, -1, "" );
		}
	}
}

void QScript::Interpret( const Chunk_t& chunk )
{
	// Compiler::DisassembleChunk( chunk, "main" );

	VM_t vm( &chunk );
	Run( vm );
}
