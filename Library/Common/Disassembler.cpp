#include "QLibPCH.h"
#include "Chunk.h"

#include "../../Includes/Instructions.h"
#include "../Compiler/Compiler.h"
#include "../Runtime/QVM.h"

bool Compiler::FindDebugSymbol( const QScript::Chunk_t& chunk, uint32_t offset, QScript::Chunk_t::Debug_t* out )
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

std::string Compiler::ValueToString( const QScript::Value& value )
{
	std::string valueType;

	std::map< QScript::ValueType, std::string > typeStrings ={
		{ QScript::ValueType::VT_NULL,		"null" },
		{ QScript::ValueType::VT_NUMBER,	"number" },
		{ QScript::ValueType::VT_BOOL,		"bool" },
	};

	switch ( value.m_Type )
	{
	case QScript::ValueType::VT_OBJECT:
	{
		if ( IS_STRING( value ) )
			valueType = "string";
		else if ( IS_FUNCTION( value ) || IS_CLOSURE( value ) )
			valueType = "function";
		else if ( IS_NATIVE( value ) )
			valueType = "native";
		else
			throw Exception( "disasm_invalid_value_object", "Invalid object type: " + std::to_string( AS_OBJECT( value )->m_Type ) );

		break;
	}
	default:
	{
		auto string =  typeStrings.find( value.m_Type );
		if ( string != typeStrings.end() )
			valueType = string->second;
		else
			throw Exception( "disasm_invalid_value", "Invalid value type: " + std::to_string( value.m_Type ) );
	}
	}

	return "(" + valueType + ", " + value.ToString() + ")";
}

void Compiler::DisassembleChunk( const QScript::Chunk_t& chunk, const std::string& identifier, int ip )
{
	// Show identifier of the current chunk
	std::cout << "=== " + identifier + " ===" << std::endl;

	// Print each instruction and their operands
	for ( size_t offset = 0; offset < chunk.m_Code.size(); )
		offset = DisassembleInstruction( chunk, offset, ip != -1 && offset == ( size_t ) ip );
}

uint32_t Compiler::DisassembleInstruction( const QScript::Chunk_t& chunk, uint32_t offset, bool isIp )
{
#define SIMPLE_INST( inst, name ) case QScript::OpCode::inst: {\
	instString = name; \
	instOffset = offset + InstructionSize( QScript::OpCode::inst ); \
	break; }

#define CNST_INST_SHORT( inst, name ) case QScript::OpCode::inst: { \
	uint8_t constant = chunk.m_Code[ offset + 1 ]; \
	const QScript::Value& value = chunk.m_Constants[ constant ]; \
	instString = name + std::string( " " ) \
		+ std::to_string( constant ) \
		+ " " + Compiler::ValueToString( value ) \
		+ " (SHORT)"; \
	instOffset = offset + InstructionSize( QScript::OpCode::inst ); \
	break; \
}

#define CNST_INST_LONG( inst, name ) case QScript::OpCode::inst: { \
	uint32_t constant = DECODE_LONG( chunk.m_Code[ offset + 1 ], chunk.m_Code[ offset + 2 ], chunk.m_Code[ offset + 3 ], chunk.m_Code[ offset + 4 ] ); \
	const QScript::Value& value = chunk.m_Constants[ constant ]; \
	instString = name + std::string( " " ) \
		+ std::to_string( constant ) \
		+ " " + Compiler::ValueToString( value ) \
		+ " (LONG)"; \
	instOffset = offset + InstructionSize( QScript::OpCode::inst ); \
	break; \
}

#define INST_LONG( inst, name ) case QScript::OpCode::inst: { \
	uint32_t value = DECODE_LONG( chunk.m_Code[ offset + 1 ], chunk.m_Code[ offset + 2 ], chunk.m_Code[ offset + 3 ], chunk.m_Code[ offset + 4 ] ); \
	instString = name + std::string( " " ) \
		+ std::to_string( value ) \
		+ " (LONG)"; \
	instOffset = offset + InstructionSize( QScript::OpCode::inst ); \
	break; \
}

#define INST_SHORT( inst, name ) case QScript::OpCode::inst: { \
	uint8_t value = chunk.m_Code[ offset + 1 ]; \
	instString = name + std::string( " " ) \
		+ std::to_string( value ) \
		+ " (SHORT)"; \
	instOffset = offset + InstructionSize( QScript::OpCode::inst ); \
	break; \
}

#define JMP_INST_LONG( inst, name, backwards ) case QScript::OpCode::inst: { \
	uint32_t value = DECODE_LONG( chunk.m_Code[ offset + 1 ], chunk.m_Code[ offset + 2 ], chunk.m_Code[ offset + 3 ], chunk.m_Code[ offset + 4 ] ); \
	instString = name + std::string( " " ) \
		+ std::to_string( value ) \
		+ " [to " + std::to_string( backwards ? ( offset - value ) : ( offset + 5 + value ) ) + "]" \
		+ " (LONG)"; \
	instOffset = offset + InstructionSize( QScript::OpCode::inst ); \
	break; \
}

#define JMP_INST_SHORT( inst, name, backwards ) case QScript::OpCode::inst: { \
	uint8_t value = chunk.m_Code[ offset + 1 ]; \
	instString = name + std::string( " " ) \
		+ std::to_string( value ) \
		+ " [to " + std::to_string( backwards ? ( offset - value ) : ( offset + 2 + value ) ) + "]" \
		+ " (SHORT)"; \
	instOffset = offset + InstructionSize( QScript::OpCode::inst ); \
	break; \
}

	// Print offset with 3 leading zeros
	std::cout << std::setfill( '0' ) << std::setw( 4 ) << offset << " "
		<< std::setw( 0 ) << std::setfill( ' ' );

	QScript::Chunk_t::Debug_t debug;
	bool hasSymbols = FindDebugSymbol( chunk, offset, &debug );

	std::string debugString = "[" + ( hasSymbols
		? std::to_string( debug.m_Line ) + ", " + std::to_string( debug.m_Column ) + ", \"" + debug.m_Token + "\""
		: "-,-,-" ) + "]";

	std::string instString;
	uint32_t instOffset;

	// Decode current instruction
	switch ( chunk.m_Code[ offset ] )
	{
	CNST_INST_SHORT( OP_LOAD_CONSTANT_SHORT, "LOAD_CONSTANT" );
	CNST_INST_LONG( OP_LOAD_CONSTANT_LONG, "LOAD_CONSTANT" );
	CNST_INST_SHORT( OP_SET_GLOBAL_SHORT, "SET_GLOBAL" );
	CNST_INST_LONG( OP_SET_GLOBAL_LONG, "SET_GLOBAL" );
	CNST_INST_SHORT( OP_LOAD_GLOBAL_SHORT, "LOAD_GLOBAL" );
	CNST_INST_LONG( OP_LOAD_GLOBAL_LONG, "LOAD_GLOBAL" );
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
	SIMPLE_INST( OP_CLOSE_UPVALUE, "CLOSE_UPVALUE" );
	SIMPLE_INST( OP_ADD, "ADD" );
	SIMPLE_INST( OP_SUB, "SUB" );
	SIMPLE_INST( OP_DIV, "DIV" );
	SIMPLE_INST( OP_MUL, "MUL" );
	SIMPLE_INST( OP_NEGATE, "NEGATE" );
	SIMPLE_INST( OP_NOT, "NOT" );
	SIMPLE_INST( OP_EQUALS, "EQUALS" );
	SIMPLE_INST( OP_NOT_EQUALS, "NOT_EQUALS" );
	SIMPLE_INST( OP_GREATERTHAN, "GREATERTHAN" );
	SIMPLE_INST( OP_LESSTHAN, "LESSTHAN" );
	SIMPLE_INST( OP_GREATERTHAN_OR_EQUAL, "GREATERTHAN_OR_EQUAL" );
	SIMPLE_INST( OP_LESSTHAN_OR_EQUAL, "LESSTHAN_OR_EQUAL" );
	SIMPLE_INST( OP_RETURN, "RETUN" );
	SIMPLE_INST( OP_PRINT, "PRINT" );
	SIMPLE_INST( OP_POP, "POP" );
	SIMPLE_INST( OP_LOAD_NULL, "LOAD_NULL" );
	case QScript::OpCode::OP_CLOSURE_SHORT:
	{
		uint32_t constant = ( uint32_t ) chunk.m_Code[ offset + 1 ];

		const QScript::Value& function = chunk.m_Constants[ constant ];
		instString = "CLOSURE " + Compiler::ValueToString( function ) + " (SHORT)";

		auto functionObj = AS_FUNCTION( function );

		for ( int i = 0; i < functionObj->GetProperties()->m_NumUpvalues; ++i )
		{
			uint32_t upvalueOffset = offset + InstructionSize( QScript::OpCode::OP_CLOSURE_SHORT ) + ( i * 5 );

			bool isLocal = chunk.m_Code[ upvalueOffset ] == 1;
			uint32_t upvalue = DECODE_LONG( chunk.m_Code[ upvalueOffset + 1 ], chunk.m_Code[ upvalueOffset + 2 ],
				chunk.m_Code[ upvalueOffset + 3 ], chunk.m_Code[ upvalueOffset + 4 ]);

			instString += "\n\t Capture: " + std::to_string( upvalue ) + " (" + ( isLocal ? "local" : "upval" ) + ")";
		}

		instOffset = offset + InstructionSize( QScript::OpCode::OP_CLOSURE_SHORT )
			+ ( functionObj->GetProperties()->m_NumUpvalues * 5 );
		break;
	}
	case QScript::OpCode::OP_CLOSURE_LONG:
	{
		uint32_t constant = DECODE_LONG( chunk.m_Code[ offset + 1 ], chunk.m_Code[ offset + 2 ],
			chunk.m_Code[ offset + 3 ], chunk.m_Code[ offset + 4 ] );

		const QScript::Value& function = chunk.m_Constants[ constant ];
		instString = "CLOSURE " + Compiler::ValueToString( function ) + " (LONG)";

		auto functionObj = AS_FUNCTION( function );

		for ( int i = 0; i < functionObj->GetProperties()->m_NumUpvalues; ++i )
		{
			uint8_t upvalueOffset = offset + InstructionSize( QScript::OpCode::OP_CLOSURE_LONG ) + ( i * 5 );

			bool isLocal = chunk.m_Code[ upvalueOffset ] == 1;
			uint32_t upvalue = DECODE_LONG( chunk.m_Code[ upvalueOffset + 1 ], chunk.m_Code[ upvalueOffset + 2 ],
				chunk.m_Code[ upvalueOffset + 3 ], chunk.m_Code[ upvalueOffset + 4 ]);

			instString += "\n\t Capture: " + std::to_string( upvalue ) + " (" + ( isLocal ? "local" : "upval" ) + ")";
		}

		instOffset = offset + InstructionSize( QScript::OpCode::OP_CLOSURE_LONG )
			+ ( functionObj->GetProperties()->m_NumUpvalues * 5 );
		break;
	}
	default:
		std::cout << "Unknown opcode: " << chunk.m_Code[ offset ] << std::endl;
		instOffset = offset + 1;
		break;
	}

	std::cout << std::setfill( ' ' ) << std::left << std::setw( 2 ) << ( isIp ? "> " : " " )
		<< std::setw( 45 ) << instString
		<< std::setfill( ' ' ) << std::right << std::setw( 0 ) << debugString
		<< std::endl;

#undef SIMPLE_INST
#undef CNST_INST_LONG
#undef CNST_INST_SHORT
#undef INST_LONG
#undef INST_SHORT
#undef JMP_INST_SHORT
#undef JMP_INST_LONG
	return instOffset;
}

int Compiler::InstructionSize( uint8_t inst )
{
	switch ( inst )
	{
	case QScript::OpCode::OP_LOAD_CONSTANT_SHORT: return 2;
	case QScript::OpCode::OP_LOAD_CONSTANT_LONG: return 5;
	case QScript::OpCode::OP_SET_GLOBAL_SHORT: return 2;
	case QScript::OpCode::OP_SET_GLOBAL_LONG: return 5;
	case QScript::OpCode::OP_LOAD_GLOBAL_SHORT: return 2;
	case QScript::OpCode::OP_LOAD_GLOBAL_LONG: return 5;
	case QScript::OpCode::OP_LOAD_LOCAL_SHORT: return 2;
	case QScript::OpCode::OP_LOAD_LOCAL_LONG: return 5;
	case QScript::OpCode::OP_LOAD_UPVALUE_SHORT: return 2;
	case QScript::OpCode::OP_LOAD_UPVALUE_LONG: return 5;
	case QScript::OpCode::OP_SET_LOCAL_SHORT: return 2;
	case QScript::OpCode::OP_SET_LOCAL_LONG: return 5;
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
	case QScript::OpCode::OP_CALL: return 2;
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
	case QScript::OpCode::OP_PRINT: return 1;
	case QScript::OpCode::OP_POP: return 1;
	case QScript::OpCode::OP_LOAD_NULL: return 1;
	default:
		return 1;
	}
}

void Compiler::DumpStack( const VM_t& vm )
{
	std::cout << "STACK (" << ( vm.m_StackTop - vm.m_Stack ) << ", capacity=" << vm.m_StackCapacity << ")" << std::endl;
	for ( const QScript::Value* value = vm.m_Stack; value < vm.m_StackTop; ++value )
	{
		std::cout << std::setfill( '0' ) << std::setw( 4 ) << ( value - vm.m_Stack ) << std::setfill( ' ' ) << std::left << std::setw( 10 ) << " ";
		std::cout << Compiler:: ValueToString( *value ) << std::setfill( ' ' ) << std::right << std::setw( 0 ) << std::endl;
	}
}

void Compiler::DumpConstants( const QScript::Chunk_t& chunk )
{
	std::cout << "CONSTANTS (" << ( chunk.m_Constants.size() ) << ")" << std::endl;
	for ( size_t i = 0; i < chunk.m_Constants.size(); ++i )
	{
		auto value = chunk.m_Constants[ i ];
		std::cout << std::setfill( '0' ) << std::setw( 4 ) << i << std::setfill( ' ' ) << std::left << std::setw( 10 ) << " ";
		std::cout << Compiler::ValueToString( value ) << std::setfill( ' ' ) << std::right << std::setw( 0 ) << std::endl;
	}
}

void Compiler::DumpGlobals( const VM_t& vm )
{
	std::cout << "GLOBALS (" << ( vm.m_Globals.size() ) << ")" << std::endl;
	for ( auto global : vm.m_Globals )
	{
		std::cout << global.first << std::setfill( ' ' ) << std::left << std::setw( 10 ) << " ";
		std::cout << Compiler::ValueToString( global.second ) << std::setfill( ' ' ) << std::right << std::setw( 0 ) << std::endl;
	}
}
