#include "QLibPCH.h"
#include "Chunk.h"

#include "../../Includes/Instructions.h"
#include "../Compiler/Compiler.h"
#include "../Runtime/QVM.h"

bool FindDebugSymbol( const QScript::Chunk_t& chunk, int offset, QScript::Chunk_t::Debug_t* out )
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

std::string Compiler::ValueToString( const QScript::Value& value )
{
	switch ( value.m_Type )
	{
		case QScript::VT_NUMBER:
		{
			char result[ 4 ];
			snprintf( result, sizeof( result ), "%.2f", AS_NUMBER( value ) );
			return std::string( "(number, " ) + result + ")";
		}
		case QScript::VT_BOOL:
			return std::string( "(bool, " ) + ( AS_BOOL( value ) ? "true" : "false" ) + ")";
		case QScript::VT_NULL:
			return "(null)";
		case QScript::VT_OBJECT:
		{
			auto object = AS_OBJECT( value );
			switch ( object->m_Type )
			{
				case QScript::OT_STRING:
					return "(string, \"" + static_cast< QScript::StringObject* >( object )->GetString() + "\")";
				default:
					throw Exception( "disasm_invalid_value_object", "Invalid object type: " + std::to_string( object->m_Type ) );
			}
		}
		default:
			throw Exception( "disasm_invalid_value", "Invalid value type: " + std::to_string( value.m_Type ) );
	}
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
#define SIMPLE_INST( inst, name ) case QScript::OpCode::inst: {\
	instString = name; \
	instOffset = offset + 1; \
	break; }

#define CNST_INST_SHORT( inst, name ) case QScript::OpCode::inst: { \
	uint8_t constant = chunk.m_Code[ offset + 1 ]; \
	const QScript::Value& value = chunk.m_Constants[ constant ]; \
	instString = name + std::string( " " ) \
		+ std::to_string( constant ) \
		+ " " + Compiler::ValueToString( value ) \
		+ " (SHORT)"; \
	instOffset = offset + 2; \
	break; \
}

#define CNST_INST_LONG( inst, name ) case QScript::OpCode::inst: { \
	uint32_t constant = DECODE_LONG( chunk.m_Code[ offset + 1 ], chunk.m_Code[ offset + 2 ], chunk.m_Code[ offset + 3 ], chunk.m_Code[ offset + 4 ] ); \
	const QScript::Value& value = chunk.m_Constants[ constant ]; \
	instString = name + std::string( " " ) \
		+ std::to_string( constant ) \
		+ " " + Compiler::ValueToString( value ) \
		+ " (LONG)"; \
	instOffset = offset + 5; \
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
	int instOffset;

	// Decode current instruction
	switch ( chunk.m_Code[ offset ] )
	{
	CNST_INST_SHORT( OP_LD_SHORT, "LOAD" );
	CNST_INST_LONG( OP_LD_LONG, "LOAD" );
	CNST_INST_SHORT( OP_SG_SHORT, "SG" );
	CNST_INST_LONG( OP_SG_LONG, "SG" );
	CNST_INST_SHORT( OP_LG_SHORT, "LG" );
	CNST_INST_LONG( OP_LG_LONG, "LG" );
	SIMPLE_INST( OP_ADD, "ADD" );
	SIMPLE_INST( OP_SUB, "SUB" );
	SIMPLE_INST( OP_DIV, "DIV" );
	SIMPLE_INST( OP_MUL, "MUL" );
	SIMPLE_INST( OP_NEG, "NEG" );
	SIMPLE_INST( OP_NOT, "NOT" );
	SIMPLE_INST( OP_EQ, "EQ" );
	SIMPLE_INST( OP_NEQ, "NEQ" );
	SIMPLE_INST( OP_GT, "GT" );
	SIMPLE_INST( OP_LT, "LT" );
	SIMPLE_INST( OP_GTE, "GTE" );
	SIMPLE_INST( OP_LTE, "LTE" );
	SIMPLE_INST( OP_RETN, "RETN" );
	SIMPLE_INST( OP_PRINT, "PRINT" );
	SIMPLE_INST( OP_POP, "POP" );
	SIMPLE_INST( OP_PNULL, "PNULL" );
	default:
		std::cout << "Unknown opcode: " << chunk.m_Code[ offset ] << std::endl;
		instOffset = offset + 1;
		break;
	}

	std::cout << std::setfill( ' ' ) << std::left << std::setw( 45 ) << instString
		<< std::setfill( ' ' ) << std::right << std::setw( 0 ) << debugString
		<< std::endl;

#undef SIMPLE_INST
#undef CNST_INST_LONG
#undef CNST_INST_SHORT
	return instOffset;
}

void Compiler::DumpStack( const VM_t& vm )
{
	std::cout << "STACK (" << ( vm.m_StackTop - vm.m_Stack ) << ")" << std::endl;
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
