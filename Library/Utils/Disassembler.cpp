#include "QLibPCH.h"
#include "../../Includes/Instructions.h"
#include "../Compiler/Compiler.h"
#include "../Runtime/QVM.h"

bool FindDebugSymbols( const QScript::Chunk_t& chunk, int offset, QScript::Chunk_t::Debug_t* out )
{
	for ( auto entry : chunk.m_Debug )
	{
		if ( entry.m_To > offset && entry.m_From <= offset )
		{
			*out = entry;
			return true;
		}
	}

	return false;
}

void Compiler::DisassembleChunk( const QScript::Chunk_t& chunk, const std::string& identifier )
{
	// Show identifier of the current chunk
	std::cout << "=== " + identifier + " ===" << std::endl;

	// Print each instruction and their operands
	for ( size_t offset = 0; offset < chunk.m_Code.size();)
		offset = DisassembleInstruction( chunk, offset );
}

int Compiler::DisassembleInstruction( const QScript::Chunk_t& chunk, int offset )
{
	// Print offset with 3 leading zeros
	std::cout << std::setfill( '0' ) << std::setw( 4 ) << offset << " "
		<< std::setw( 0 ) << std::setfill( ' ' );

	QScript::Chunk_t::Debug_t debug;
	bool hasSymbols = FindDebugSymbols( chunk, offset, &debug );

	std::string debugString = "[" + ( hasSymbols
		? std::to_string( debug.m_Line ) + ", " + std::to_string( debug.m_Column ) + ", \"" + debug.m_Token + "\""
		: "-,-,-" ) + "]";

	std::string instString;
	int instOffset;

	// Decode current instruction
	switch ( chunk.m_Code[ offset ] )
	{
	case QScript::OpCode::OP_CNST:
	{
		// Index of the constant from code
		uint8_t constant = chunk.m_Code[ offset + 1 ];

		instString = "CNST "
			+ std::to_string( constant )
			+ " (" + std::to_string( chunk.m_Constants[ chunk.m_Code[ offset + 1 ] ] ) + ")";

		instOffset = offset + 2;
		break;
	}
	case QScript::OpCode::OP_RETN:
	{
		instString = "RETN";
		instOffset = offset + 1;
		break;
	}
	default:
		std::cout << "Unknown opcode: " << chunk.m_Code[ offset ] << std::endl;
		instOffset = offset + 1;
		break;
	}

	std::cout << std::setfill( ' ' ) << std::left << std::setw( 25 ) << instString
		<< std::setfill( ' ' ) << std::right << std::setw( 0 ) << debugString
		<< std::endl;

	return instOffset;
}

void Compiler::DumpStack( const VM_t& vm )
{
	std::cout << "STACK (" << ( vm.m_StackTop - vm.m_Stack ) << ")" << std::endl;
	for ( const QScript::Value* value = vm.m_Stack; value < vm.m_StackTop; ++value )
	{
		std::cout << std::setfill( '0' ) << std::setw( 4 ) << ( value - vm.m_Stack ) << std::setfill( ' ' ) << std::left << std::setw( 10 ) << " ";
		std::cout << ( *value ) << std::setfill( ' ' ) << std::right << std::setw( 0 ) << std::endl;
	}
}
