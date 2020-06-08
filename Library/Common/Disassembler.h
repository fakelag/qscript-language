#pragma once

namespace Disassembler
{
	enum OpCodeType
	{
		OPCODE_INVALID = -1,
		OPCODE_SIMPLE = 0,
		OPCODE_SHORT,
		OPCODE_LONG,
	};

	struct OpCode_t
	{
		OpCode_t()
		{
			m_Type		= OPCODE_INVALID;
			m_Address	= 0;
			m_Size		= 1;
			m_LineNr	= -1;
			m_ColNr		= -1;
		}

		std::string						m_Name;			/* Name, e.g "IMPORT" */
		std::string						m_Full;			/* Full field, e.g "IMPORT 0 Time" */
		std::vector< std::string >		m_Attributes;	/* Additional fields like captures in closures */

		OpCodeType						m_Type;			/* Type of the instruction */
		int								m_Size;			/* Size of the complete instruction */
		uint32_t						m_Address;		/* Address in the current disassembly */

		int								m_LineNr;		/* Debug line number */
		int								m_ColNr;		/* Debug column number */
		std::string						m_Token;		/* Debug token string */
	};

	struct Disassembly_t
	{
		Disassembly_t( const std::string& name )
		{
			m_Name = name;
		}

		std::string						m_Name;
		std::vector< OpCode_t >			m_Opcodes;
		size_t							m_IpInst;
	};

	Disassembly_t DisassembleChunk( const QScript::Chunk_t& chunk, const std::string& identifier, int ip = -1 );
	//void DisassembleChunk( const QScript::Chunk_t& chunk, const std::string& identifier, int ip = -1 );
	// uint32_t DisassembleInstruction( const QScript::Chunk_t& chunk, uint32_t offset, bool isIp );
	OpCode_t DisassembleInstruction( const QScript::Chunk_t& chunk, uint32_t offset );
	int InstructionSize( uint8_t inst );

	void DumpConstants( const QScript::Chunk_t& chunk );
	void DumpDisassembly( const Disassembly_t& disassembly );
	void DumpGlobals( const VM_t& vm );
	void DumpStack( const VM_t& vm );

	bool FindDebugSymbol( const QScript::Chunk_t& chunk, uint32_t offset, QScript::Chunk_t::Debug_t* out );
}