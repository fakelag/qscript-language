#include "QLibPCH.h"
#include "Instructions.h"
#include "QVM.h"
#include "../Compiler/Compiler.h"

#define READ_BYTE( vm ) (*vm.m_IP++)
#define READ_CONST( vm ) (vm.m_Chunk->m_Constants[ READ_BYTE( vm ) ])

bool Run( VM_t& vm )
{
#ifdef QVM_DEBUG
	bool isStepping = true;
#endif

	for (;;)
	{
#ifdef QVM_DEBUG
		Compiler::DisassembleInstruction( *vm.m_Chunk, ( int )( vm.m_IP - ( uint8_t* ) &vm.m_Chunk->m_Code[ 0 ] ) );

		if ( isStepping )
		{
			std::string input;

			for( ;; )
			{
				std::cout << "Action (s/r/ds): ";
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
				else if ( input == "s" )
				{
					break;
				}
				else
				{
					std::cout << "Invalid action: " << input << std::endl;
				}
			}
		}
#endif

		uint8_t inst = READ_BYTE( vm );
		switch ( inst )
		{
		case QScript::OP_CNST:
		{
			auto constant = READ_CONST( vm );
			vm.Push( constant );
			break;
		}
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
