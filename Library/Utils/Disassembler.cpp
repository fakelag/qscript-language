#include "QLibPCH.h"
#include "../../Includes/Instructions.h"
#include "../Compiler/Compiler.h"

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
	std::cout << std::setfill( '0' ) << std::setw( 4 ) << offset << " ";

	// Decode current instruction
	switch ( chunk.m_Code[ offset ] )
	{
	case QScript::OpCode::OP_CNST:
	{
		// Index of the constant from code
		uint8_t constant = chunk.m_Code[ offset + 1 ];

		std::cout << "CNST "
			<< std::to_string( constant )
			<< " (" << chunk.m_Constants[ chunk.m_Code[ offset + 1 ] ] << ")" << std::endl;
		return offset + 2;
	}
	case QScript::OpCode::OP_RETN: std::cout << "RETN" << std::endl; return offset + 1;
	default:
		std::cout << "Unknown opcode: " << chunk.m_Code[ offset ] << std::endl;
		return offset + 1;
	}
}
