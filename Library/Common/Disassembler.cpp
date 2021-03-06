#include "QLibPCH.h"
#include "Chunk.h"

#include "../../Includes/Instructions.h"
#include "../Compiler/Compiler.h"
#include "../Runtime/QVM.h"

#include "../Common/Disassembler.h"

namespace Disassembler
{
	bool FindDebugSymbol( const QScript::Chunk_t& chunk, uint32_t offset, QScript::Chunk_t::Debug_t* out )
	{
		bool found = false;
		QScript::Chunk_t::Debug_t symbol;

		for ( auto entry : chunk.m_Debug )
		{
			if ( entry.m_To > offset && entry.m_From <= offset )
			{
				if ( found && symbol.m_To - symbol.m_From < entry.m_To - entry.m_From )
					continue;

				symbol = entry;
				found = true;
			}
		}

		if ( found )
		{
			*out = symbol;
			return true;
		}

		return false;
	}

	Disassembly_t DisassembleChunk( const QScript::Chunk_t& chunk, const std::string& identifier, int ip )
	{
		Disassembly_t disasm( identifier );

		for ( size_t offset = 0; offset < chunk.m_Code.size(); )
		{
			auto opcode = DisassembleInstruction( chunk, ( uint32_t ) offset );

			if ( offset == ( size_t ) ip )
				disasm.m_IpInst = disasm.m_Opcodes.size();

			disasm.m_Opcodes.push_back( opcode );

			offset += opcode.m_Size;
		}

		return disasm;
	}

	OpCode_t DisassembleInstruction( const QScript::Chunk_t& chunk, uint32_t offset )
	{
		OpCode_t opcode;
		opcode.m_Address = offset;
		opcode.m_Size = InstructionSize( chunk.m_Code[ offset ] );

		#define SIMPLE_INST( inst, name ) case QScript::OpCode::inst: {\
				opcode.m_Name = name; \
				opcode.m_Full = name; \
				opcode.m_Type = OPCODE_SIMPLE; \
				break; }
		
		#define CNST_INST_SHORT( inst, name ) case QScript::OpCode::inst: { \
				uint8_t constant = chunk.m_Code[ offset + 1 ]; \
				const QScript::Value& value = chunk.m_Constants[ constant ]; \
				opcode.m_Name = name; \
				opcode.m_Full = name + std::string( " " ) + std::to_string( constant ) + " " + value.ToString(); \
				opcode.m_Type = OPCODE_SHORT; \
				break; \
			}
		
		#define CNST_INST_LONG( inst, name ) case QScript::OpCode::inst: { \
				uint32_t constant = DECODE_LONG( chunk.m_Code[ offset + 1 ], chunk.m_Code[ offset + 2 ], chunk.m_Code[ offset + 3 ], chunk.m_Code[ offset + 4 ] ); \
				const QScript::Value& value = chunk.m_Constants[ constant ]; \
				opcode.m_Name = name; \
				opcode.m_Full = name + std::string( " " ) + std::to_string( constant ) + " " + value.ToString(); \
				opcode.m_Type = OPCODE_LONG; \
				break; \
			}
		
		#define INST_LONG( inst, name ) case QScript::OpCode::inst: { \
				uint32_t value = DECODE_LONG( chunk.m_Code[ offset + 1 ], chunk.m_Code[ offset + 2 ], chunk.m_Code[ offset + 3 ], chunk.m_Code[ offset + 4 ] ); \
				opcode.m_Name = name; \
				opcode.m_Full = name + std::string( " " ) + std::to_string( value ); \
				opcode.m_Type = OPCODE_LONG; \
				break; \
			}
		
		#define INST_SHORT( inst, name ) case QScript::OpCode::inst: { \
				uint8_t value = chunk.m_Code[ offset + 1 ]; \
				opcode.m_Name = name; \
				opcode.m_Full = name + std::string( " " ) + std::to_string( value ); \
				opcode.m_Type = OPCODE_SHORT; \
				break; \
			}
		
		#define JMP_INST_LONG( inst, name, backwards ) case QScript::OpCode::inst: { \
				uint32_t value = DECODE_LONG( chunk.m_Code[ offset + 1 ], chunk.m_Code[ offset + 2 ], chunk.m_Code[ offset + 3 ], chunk.m_Code[ offset + 4 ] ); \
				auto jumpTo = std::to_string( backwards ? ( offset - value ) : ( offset + 5 + value ) ); \
				opcode.m_Name = name; \
				opcode.m_Full = name + std::string( " " ) + std::to_string( value ) + " [to " + jumpTo + "]"; \
				opcode.m_Attributes.emplace_back( std::string( "{ \"type\": \"jump\", \"jumpTo\": " ) \
					+ jumpTo + ",\"relative\": " + std::to_string( value ) + ", \"dir\": " + ( backwards ? "-1" : "0" ) + "}" ); \
				opcode.m_Type = OPCODE_LONG; \
				break; \
			}
		
		#define JMP_INST_SHORT( inst, name, backwards ) case QScript::OpCode::inst: { \
				uint8_t value = chunk.m_Code[ offset + 1 ]; \
				auto jumpTo = std::to_string( backwards ? ( offset - value ) : ( offset + 2 + value ) ); \
				opcode.m_Name = name; \
				opcode.m_Full = name + std::string( " " ) + std::to_string( value ) + " [to " + jumpTo + "]"; \
				opcode.m_Attributes.emplace_back( std::string( "{ \"type\": \"jump\", \"jumpTo\": " ) \
					+ jumpTo + ",\"relative\": " + std::to_string( value ) + ", \"dir\": " + ( backwards ? "-1" : "0" ) + "}" ); \
				opcode.m_Type = OPCODE_SHORT; \
				break; \
			}
		
		
		QScript::Chunk_t::Debug_t debug;
		if ( FindDebugSymbol( chunk, offset, &debug ) )
		{
			opcode.m_LineNr = debug.m_Line;
			opcode.m_ColNr = debug.m_Column;
			opcode.m_Token = debug.m_Token;
		}
		
		// Decode current instruction
		switch ( chunk.m_Code[ offset ] )
		{
			INST_SHORT( OP_LOAD_TOP_SHORT, "LOAD_TOP" );
			CNST_INST_SHORT( OP_CREATE_ARRAY_SHORT, "CREATE_ARRAY" );
			CNST_INST_LONG( OP_CREATE_ARRAY_LONG, "CREATE_ARRAY" );
			CNST_INST_SHORT( OP_CREATE_TABLE_SHORT, "CREATE_TABLE" );
			CNST_INST_LONG( OP_CREATE_TABLE_LONG, "CREATE_TABLE" );
			CNST_INST_SHORT( OP_LOAD_CONSTANT_SHORT, "LOAD_CONSTANT" );
			CNST_INST_LONG( OP_LOAD_CONSTANT_LONG, "LOAD_CONSTANT" );
			CNST_INST_SHORT( OP_SET_GLOBAL_SHORT, "SET_GLOBAL" );
			CNST_INST_LONG( OP_SET_GLOBAL_LONG, "SET_GLOBAL" );
			CNST_INST_SHORT( OP_LOAD_GLOBAL_SHORT, "LOAD_GLOBAL" );
			CNST_INST_LONG( OP_LOAD_GLOBAL_LONG, "LOAD_GLOBAL" );
			CNST_INST_SHORT( OP_SET_PROP_SHORT, "SET_PROP" );
			CNST_INST_LONG( OP_SET_PROP_LONG, "SET_PROP" );
			CNST_INST_SHORT( OP_LOAD_PROP_SHORT, "LOAD_PROP" );
			CNST_INST_LONG( OP_LOAD_PROP_LONG, "LOAD_PROP" );
			CNST_INST_LONG( OP_IMPORT, "IMPORT" );
			INST_SHORT( OP_LOAD_LOCAL_SHORT, "LOAD_LOCAL" );
			INST_LONG( OP_LOAD_LOCAL_LONG, "LOAD_LOCAL" );
			INST_SHORT( OP_SET_LOCAL_SHORT, "SET_LOCAL" );
			INST_LONG( OP_SET_LOCAL_LONG, "SET_LOCAL" );
			INST_SHORT( OP_LOAD_UPVALUE_SHORT, "LOAD_UPVALUE" );
			INST_LONG( OP_LOAD_UPVALUE_LONG, "LOAD_UPVALUE" );
			INST_SHORT( OP_SET_UPVALUE_SHORT, "SET_UPVALUE" );
			INST_LONG( OP_SET_UPVALUE_LONG, "SET_UPVALUE" );
			JMP_INST_SHORT( OP_JUMP_IF_ZERO_SHORT, "JUMP_IF_ZERO", false );
			JMP_INST_LONG( OP_JUMP_IF_ZERO_LONG, "JUMP_IF_ZERO", false );
			JMP_INST_SHORT( OP_JUMP_SHORT, "JUMP", false );
			JMP_INST_LONG( OP_JUMP_LONG, "JUMP", false );
			JMP_INST_SHORT( OP_JUMP_BACK_SHORT, "JUMP_BACK", true );
			JMP_INST_LONG( OP_JUMP_BACK_LONG, "JUMP_BACK", true );
			INST_SHORT( OP_CALL, "CALL" );
			SIMPLE_INST( OP_CALL_0, "CALL 0" );
			SIMPLE_INST( OP_CALL_1, "CALL 1" );
			SIMPLE_INST( OP_CALL_2, "CALL 2" );
			SIMPLE_INST( OP_CALL_3, "CALL 3" );
			SIMPLE_INST( OP_CALL_4, "CALL 4" );
			SIMPLE_INST( OP_CALL_5, "CALL 5" );
			SIMPLE_INST( OP_CALL_6, "CALL 6" );
			SIMPLE_INST( OP_CALL_7, "CALL 7" );
			SIMPLE_INST( OP_LOAD_LOCAL_0, "LOAD_LOCAL 0" );
			SIMPLE_INST( OP_LOAD_LOCAL_1, "LOAD_LOCAL 1" );
			SIMPLE_INST( OP_LOAD_LOCAL_2, "LOAD_LOCAL 2" );
			SIMPLE_INST( OP_LOAD_LOCAL_3, "LOAD_LOCAL 3" );
			SIMPLE_INST( OP_LOAD_LOCAL_4, "LOAD_LOCAL 4" );
			SIMPLE_INST( OP_LOAD_LOCAL_5, "LOAD_LOCAL 5" );
			SIMPLE_INST( OP_LOAD_LOCAL_6, "LOAD_LOCAL 6" );
			SIMPLE_INST( OP_LOAD_LOCAL_7, "LOAD_LOCAL 7" );
			SIMPLE_INST( OP_LOAD_LOCAL_8, "LOAD_LOCAL 8" );
			SIMPLE_INST( OP_LOAD_LOCAL_9, "LOAD_LOCAL 9" );
			SIMPLE_INST( OP_LOAD_LOCAL_10, "LOAD_LOCAL 10" );
			SIMPLE_INST( OP_LOAD_LOCAL_11, "LOAD_LOCAL 11" );
			SIMPLE_INST( OP_LOAD_PROP_STACK, "LOAD_PROP_STACK" );
			SIMPLE_INST( OP_PUSH_ARRAY, "OP_PUSH_ARRAY" );
			SIMPLE_INST( OP_SET_LOCAL_0, "SET_LOCAL 0" );
			SIMPLE_INST( OP_SET_LOCAL_1, "SET_LOCAL 1" );
			SIMPLE_INST( OP_SET_LOCAL_2, "SET_LOCAL 2" );
			SIMPLE_INST( OP_SET_LOCAL_3, "SET_LOCAL 3" );
			SIMPLE_INST( OP_SET_LOCAL_4, "SET_LOCAL 4" );
			SIMPLE_INST( OP_SET_LOCAL_5, "SET_LOCAL 5" );
			SIMPLE_INST( OP_SET_LOCAL_6, "SET_LOCAL 6" );
			SIMPLE_INST( OP_SET_LOCAL_7, "SET_LOCAL 7" );
			SIMPLE_INST( OP_SET_LOCAL_8, "SET_LOCAL 8" );
			SIMPLE_INST( OP_SET_LOCAL_9, "SET_LOCAL 9" );
			SIMPLE_INST( OP_SET_LOCAL_10, "SET_LOCAL 10" );
			SIMPLE_INST( OP_SET_LOCAL_11, "SET_LOCAL 11" );
			SIMPLE_INST( OP_SET_PROP_STACK, "LOAD_SET_STACK" );
			SIMPLE_INST( OP_CLOSE_UPVALUE, "CLOSE_UPVALUE" );
			SIMPLE_INST( OP_ADD, "ADD" );
			SIMPLE_INST( OP_BIND, "BIND" );
			SIMPLE_INST( OP_SUB, "SUB" );
			SIMPLE_INST( OP_DIV, "DIV" );
			SIMPLE_INST( OP_MUL, "MUL" );
			SIMPLE_INST( OP_POW, "POW" );
			SIMPLE_INST( OP_MOD, "MOD" );
			SIMPLE_INST( OP_NEGATE, "NEGATE" );
			SIMPLE_INST( OP_NOT, "NOT" );
			SIMPLE_INST( OP_EQUALS, "EQUALS" );
			SIMPLE_INST( OP_NOT_EQUALS, "NOT_EQUALS" );
			SIMPLE_INST( OP_GREATERTHAN, "GREATERTHAN" );
			SIMPLE_INST( OP_LESSTHAN, "LESSTHAN" );
			SIMPLE_INST( OP_GREATERTHAN_OR_EQUAL, "GREATERTHAN_OR_EQUAL" );
			SIMPLE_INST( OP_LESSTHAN_OR_EQUAL, "LESSTHAN_OR_EQUAL" );
			SIMPLE_INST( OP_RETURN, "RETUN" );
			SIMPLE_INST( OP_POP, "POP" );
			SIMPLE_INST( OP_LOAD_NULL, "LOAD_NULL" );
			SIMPLE_INST( OP_LOAD_MINUS_1, "LOAD_NEG_1" );
			SIMPLE_INST( OP_LOAD_0, "LOAD_0" );
			SIMPLE_INST( OP_LOAD_1, "LOAD_1" );
			SIMPLE_INST( OP_LOAD_2, "LOAD_2" );
			SIMPLE_INST( OP_LOAD_3, "LOAD_3" );
			SIMPLE_INST( OP_LOAD_4, "LOAD_4" );
			SIMPLE_INST( OP_LOAD_5, "LOAD_5" );
		case QScript::OpCode::OP_CLOSURE_SHORT:
		{
			uint32_t constant = ( uint32_t ) chunk.m_Code[ offset + 1 ];
		
			const QScript::Value& function = chunk.m_Constants[ constant ];


			opcode.m_Name = "CLOSURE";
			opcode.m_Full = "CLOSURE " + function.ToString();
			opcode.m_Type = OPCODE_SHORT;
		
			auto functionObj = AS_FUNCTION( function );
		
			for ( int i = 0; i < functionObj->NumUpvalues(); ++i )
			{
				uint32_t upvalueOffset = offset + InstructionSize( QScript::OpCode::OP_CLOSURE_SHORT ) + ( i * 5 );
		
				bool isLocal = chunk.m_Code[ upvalueOffset ] == 1;
				uint32_t upvalue = DECODE_LONG( chunk.m_Code[ upvalueOffset + 1 ], chunk.m_Code[ upvalueOffset + 2 ],
					chunk.m_Code[ upvalueOffset + 3 ], chunk.m_Code[ upvalueOffset + 4 ] );
		
				opcode.m_Attributes.emplace_back( "{ \"type\": \"capture\", \"upvalueIndex:\": "
					+ std::to_string( upvalue ) + ", \"isLocal\": " + ( isLocal ? "true" : "false" ) + "}" );
			}
			break;
		}
		case QScript::OpCode::OP_CLOSURE_LONG:
		{
			uint32_t constant = DECODE_LONG( chunk.m_Code[ offset + 1 ], chunk.m_Code[ offset + 2 ],
				chunk.m_Code[ offset + 3 ], chunk.m_Code[ offset + 4 ] );
		
			const QScript::Value& function = chunk.m_Constants[ constant ];

			opcode.m_Name = "CLOSURE";
			opcode.m_Full = "CLOSURE " + function.ToString();
			opcode.m_Type = OPCODE_LONG;
		
			auto functionObj = AS_FUNCTION( function );
		
			for ( int i = 0; i < functionObj->NumUpvalues(); ++i )
			{
				uint8_t upvalueOffset = offset + InstructionSize( QScript::OpCode::OP_CLOSURE_LONG ) + ( i * 5 );
		
				bool isLocal = chunk.m_Code[ upvalueOffset ] == 1;
				uint32_t upvalue = DECODE_LONG( chunk.m_Code[ upvalueOffset + 1 ], chunk.m_Code[ upvalueOffset + 2 ],
					chunk.m_Code[ upvalueOffset + 3 ], chunk.m_Code[ upvalueOffset + 4 ] );
		
				opcode.m_Attributes.emplace_back( "{ \"type\": \"capture\", \"upvalueIndex:\": "
					+ std::to_string( upvalue ) + ", \"isLocal\": " + ( isLocal ? "true" : "false" ) + "}" );
			}
		
			break;
		}
		default:
			throw Exception( "disasm_unknown_opcode", "Unknown opcode: " + std::to_string( chunk.m_Code[ offset ] ) );
		}
		
		#undef SIMPLE_INST
		#undef CNST_INST_LONG
		#undef CNST_INST_SHORT
		#undef INST_LONG
		#undef INST_SHORT
		#undef JMP_INST_SHORT
		#undef JMP_INST_LONG
		return opcode;
	}

	int InstructionSize( uint8_t inst )
	{
		switch ( inst )
		{
		case QScript::OpCode::OP_BIND: return 1;
		case QScript::OpCode::OP_PUSH_ARRAY: return 1;
		case QScript::OpCode::OP_LOAD_PROP_STACK: return 1;
		case QScript::OpCode::OP_SET_PROP_STACK: return 1;
		case QScript::OpCode::OP_LOAD_TOP_SHORT: return 2;
		case QScript::OpCode::OP_CREATE_TABLE_SHORT: return 2;
		case QScript::OpCode::OP_CREATE_TABLE_LONG: return 5;
		case QScript::OpCode::OP_CREATE_ARRAY_SHORT: return 2;
		case QScript::OpCode::OP_CREATE_ARRAY_LONG: return 5;
		case QScript::OpCode::OP_LOAD_CONSTANT_SHORT: return 2;
		case QScript::OpCode::OP_LOAD_CONSTANT_LONG: return 5;
		case QScript::OpCode::OP_SET_GLOBAL_SHORT: return 2;
		case QScript::OpCode::OP_SET_GLOBAL_LONG: return 5;
		case QScript::OpCode::OP_LOAD_GLOBAL_SHORT: return 2;
		case QScript::OpCode::OP_LOAD_GLOBAL_LONG: return 5;
		case QScript::OpCode::OP_LOAD_LOCAL_SHORT: return 2;
		case QScript::OpCode::OP_LOAD_LOCAL_LONG: return 5;
		case QScript::OpCode::OP_LOAD_PROP_SHORT: return 2;
		case QScript::OpCode::OP_LOAD_PROP_LONG: return 5;
		case QScript::OpCode::OP_LOAD_UPVALUE_SHORT: return 2;
		case QScript::OpCode::OP_LOAD_UPVALUE_LONG: return 5;
		case QScript::OpCode::OP_SET_LOCAL_SHORT: return 2;
		case QScript::OpCode::OP_SET_LOCAL_LONG: return 5;
		case QScript::OpCode::OP_SET_PROP_SHORT: return 2;
		case QScript::OpCode::OP_SET_PROP_LONG: return 5;
		case QScript::OpCode::OP_SET_UPVALUE_SHORT: return 2;
		case QScript::OpCode::OP_SET_UPVALUE_LONG: return 5;
		case QScript::OpCode::OP_JUMP_IF_ZERO_SHORT: return 2;
		case QScript::OpCode::OP_JUMP_IF_ZERO_LONG: return 5;
		case QScript::OpCode::OP_JUMP_SHORT: return 2;
		case QScript::OpCode::OP_JUMP_LONG: return 5;
		case QScript::OpCode::OP_JUMP_BACK_SHORT: return 2;
		case QScript::OpCode::OP_JUMP_BACK_LONG: return 5;
		case QScript::OpCode::OP_CLOSE_UPVALUE: return 1;
		case QScript::OpCode::OP_ADD: return 1;
		case QScript::OpCode::OP_SUB: return 1;
		case QScript::OpCode::OP_DIV: return 1;
		case QScript::OpCode::OP_MUL: return 1;
		case QScript::OpCode::OP_POW: return 1;
		case QScript::OpCode::OP_MOD: return 1;
		case QScript::OpCode::OP_CALL: return 2;
		case QScript::OpCode::OP_IMPORT: return 5;
		case QScript::OpCode::OP_CLOSURE_LONG: return 5;
		case QScript::OpCode::OP_CLOSURE_SHORT: return 2;
		case QScript::OpCode::OP_NEGATE: return 1;
		case QScript::OpCode::OP_NOT: return 1;
		case QScript::OpCode::OP_EQUALS: return 1;
		case QScript::OpCode::OP_NOT_EQUALS: return 1;
		case QScript::OpCode::OP_GREATERTHAN: return 1;
		case QScript::OpCode::OP_LESSTHAN: return 1;
		case QScript::OpCode::OP_GREATERTHAN_OR_EQUAL: return 1;
		case QScript::OpCode::OP_LESSTHAN_OR_EQUAL: return 1;
		case QScript::OpCode::OP_RETURN: return 1;
		case QScript::OpCode::OP_POP: return 1;
		case QScript::OpCode::OP_LOAD_NULL:
		case QScript::OpCode::OP_LOAD_MINUS_1:
		case QScript::OpCode::OP_LOAD_0:
		case QScript::OpCode::OP_LOAD_1:
		case QScript::OpCode::OP_LOAD_2:
		case QScript::OpCode::OP_LOAD_3:
		case QScript::OpCode::OP_LOAD_4:
		case QScript::OpCode::OP_LOAD_5:
			return 1;
		case QScript::OpCode::OP_CALL_0:
		case QScript::OpCode::OP_CALL_1:
		case QScript::OpCode::OP_CALL_2:
		case QScript::OpCode::OP_CALL_3:
		case QScript::OpCode::OP_CALL_4:
		case QScript::OpCode::OP_CALL_5:
		case QScript::OpCode::OP_CALL_6:
		case QScript::OpCode::OP_CALL_7:
			return 1;
		case QScript::OpCode::OP_LOAD_LOCAL_0:
		case QScript::OpCode::OP_LOAD_LOCAL_1:
		case QScript::OpCode::OP_LOAD_LOCAL_2:
		case QScript::OpCode::OP_LOAD_LOCAL_3:
		case QScript::OpCode::OP_LOAD_LOCAL_4:
		case QScript::OpCode::OP_LOAD_LOCAL_5:
		case QScript::OpCode::OP_LOAD_LOCAL_6:
		case QScript::OpCode::OP_LOAD_LOCAL_7:
		case QScript::OpCode::OP_LOAD_LOCAL_8:
		case QScript::OpCode::OP_LOAD_LOCAL_9:
		case QScript::OpCode::OP_LOAD_LOCAL_10:
		case QScript::OpCode::OP_LOAD_LOCAL_11:
			return 1;
		case QScript::OpCode::OP_SET_LOCAL_0:
		case QScript::OpCode::OP_SET_LOCAL_1:
		case QScript::OpCode::OP_SET_LOCAL_2:
		case QScript::OpCode::OP_SET_LOCAL_3:
		case QScript::OpCode::OP_SET_LOCAL_4:
		case QScript::OpCode::OP_SET_LOCAL_5:
		case QScript::OpCode::OP_SET_LOCAL_6:
		case QScript::OpCode::OP_SET_LOCAL_7:
		case QScript::OpCode::OP_SET_LOCAL_8:
		case QScript::OpCode::OP_SET_LOCAL_9:
		case QScript::OpCode::OP_SET_LOCAL_10:
		case QScript::OpCode::OP_SET_LOCAL_11:
			return 1;
		default:
			return 1;
		}
	}

	void DumpDisassembly( const Disassembly_t& disassembly )
	{
		std::cout << "=== " + disassembly.m_Name + " ===" << std::endl;

		for ( size_t instIndex = 0; instIndex < disassembly.m_Opcodes.size(); ++instIndex )
		{
			auto& opcode = disassembly.m_Opcodes[ instIndex ];
			bool isIp = disassembly.m_IpInst == instIndex;
			std::string symbols = "";
			std::string opCodeType = "";

			switch ( opcode.m_Type )
			{
			case OPCODE_LONG: opCodeType = "(LONG)"; break;
			case OPCODE_SHORT: opCodeType = "(SHORT)"; break;
			default: break;
			}

			if ( opcode.m_LineNr != -1 )
			{
				symbols = "[" + std::to_string( opcode.m_LineNr ) + ", "
					+ std::to_string( opcode.m_ColNr ) + ", \"" + opcode.m_Token + "\"]";
			}

			// Print address with 3 leading zeros
			std::cout << std::setfill( '0' ) << std::setw( 4 ) << opcode.m_Address << " "
				<< std::setw( 0 ) << std::setfill( ' ' );

			// Print main instruction
			std::cout << std::setfill( ' ' ) << std::left << std::setw( 2 ) << ( isIp ? "> " : " " )
				<< std::setw( 45 ) << opcode.m_Full + " " + opCodeType;

			// Print debugging symbols
			std::cout << std::setfill( ' ' ) << std::right << std::setw( 0 ) << symbols
				<< std::endl;
		}
	}

	void DumpStack( const VM_t& vm )
	{
		std::cout << "STACK (" << ( vm.m_StackTop - vm.m_Stack ) << ", capacity=" << vm.m_StackCapacity << ")" << std::endl;
		for ( const QScript::Value* value = vm.m_Stack; value < vm.m_StackTop; ++value )
		{
			std::cout << std::setfill( '0' ) << std::setw( 4 ) << ( value - vm.m_Stack ) << std::setfill( ' ' ) << std::left << std::setw( 10 ) << " ";
			std::cout << value->ToString() << std::setfill( ' ' ) << std::right << std::setw( 0 ) << std::endl;
		}
	}

	void DumpConstants( const QScript::Chunk_t& chunk )
	{
		std::cout << "CONSTANTS (" << ( chunk.m_Constants.size() ) << ")" << std::endl;
		for ( size_t i = 0; i < chunk.m_Constants.size(); ++i )
		{
			auto value = chunk.m_Constants[ i ];
			std::cout << std::setfill( '0' ) << std::setw( 4 ) << i << std::setfill( ' ' ) << std::left << std::setw( 10 ) << " ";
			std::cout << value.ToString() << std::setfill( ' ' ) << std::right << std::setw( 0 ) << std::endl;
		}
	}

	void DumpGlobals( const VM_t& vm )
	{
		std::cout << "GLOBALS (" << ( vm.m_Globals.size() ) << ")" << std::endl;
		for ( auto global : vm.m_Globals )
		{
			std::cout << std::setfill( ' ' ) << std::left << std::setw( 25 ) << global.first << " ";
			std::cout << std::setfill( ' ' ) << std::right << std::setw( 0 ) << global.second.ToString() << std::endl;
		}
	}
}
