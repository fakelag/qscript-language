#include "QLibPCH.h"
#include "Instructions.h"
#include "QVM.h"
#include "../Compiler/Compiler.h"

#define READ_BYTE( vm ) (*vm.m_IP++)
#define READ_CONST( vm ) (vm.m_Chunk->m_Constants[ READ_BYTE( vm ) ])
#define BINARY_OP( op ) { auto b = vm.Pop(); auto a = vm.Pop(); vm.Push( a op b ); }

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
		case QScript::OP_NEG: vm.Push( -vm.Pop() ); break;
		case QScript::OP_ADD: BINARY_OP( + ); break;
		case QScript::OP_SUB: BINARY_OP( - ); break;
		case QScript::OP_MUL: BINARY_OP( * ); break;
		case QScript::OP_DIV: BINARY_OP( / ); break;
		case QScript::OP_RETN:
		{
			std::cout << "Exit: " << ( vm.Pop() ) << std::endl;
			return true;
		}
		}
	}
}

void QScript::Interpret( const Chunk_t& chunk )
{
	// Compiler::DisassembleChunk( chunk, "main" );

	VM_t vm( &chunk );
	Run( vm );
}
