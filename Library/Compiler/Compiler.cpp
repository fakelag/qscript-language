#include "QLibPCH.h"
#include "../Common/Chunk.h"
#include "../STL/NativeModule.h"

#include "Instructions.h"
#include "Compiler.h"
#include "Types.h"

#define BEGIN_COMPILER \
Object::AllocateString = &Compiler::AllocateString; \
Object::AllocateFunction = &Compiler::AllocateFunction; \
Object::AllocateNative = &Compiler::AllocateNative; \
Object::AllocateClosure = &Compiler::AllocateClosure; \
Object::AllocateUpvalue = &Compiler::AllocateUpvalue; \
Object::AllocateTable = &Compiler::AllocateTable; \
Object::AllocateArray = &Compiler::AllocateArray;

#define END_COMPILER \
Object::AllocateString = NULL; \
Object::AllocateFunction = NULL; \
Object::AllocateNative = NULL; \
Object::AllocateClosure = NULL; \
Object::AllocateUpvalue = NULL; \
Object::AllocateTable = NULL; \
Object::AllocateArray = NULL;

namespace QScript
{
	FunctionObject* Compile( const std::string& source, const Config_t& config )
	{
		BEGIN_COMPILER;

		// Make sure module system is initialized
		QScript::InitModules();

		Chunk_t* chunk = AllocChunk();
		Compiler::Assembler assembler( chunk, config );

		// Import main system module (functions like exit(), print(), etc)
		auto systemModule = QScript::ResolveModule( "System" );
		systemModule->Import( &assembler );

		std::vector< Compiler::BaseNode* > astNodes;

		try
		{
			// Lexical analysis (tokenization)
			auto tokens = Compiler::Lexer( source );

			// Generate IR
			astNodes = Compiler::GenerateIR( tokens );

			// Run IR optimizers

			// Compile bytecode
			for ( auto node : astNodes )
				node->Compile( assembler );

			for ( auto node : astNodes )
			{
				node->Release();
				delete node;
			}

			astNodes.clear();

			// Compiled funtions
			auto functions = assembler.Finish();

			// Clean up objects created in compilation process
			Compiler::GarbageCollect( functions );

			// Reset allocators
			END_COMPILER;

			// Return compiled code
			return functions.back();
		}
		catch ( const CompilerException& exception )
		{
			// Free created objects
			Compiler::GarbageCollect( std::vector<QScript::FunctionObject*>{ } );

			// Free compilation materials (also frees main chunk)
			assembler.Release();

			// Free AST
			for ( auto node : astNodes )
			{
				node->Release();
				delete node;
			}
			astNodes.clear();

			// Rethrow
			throw std::vector< CompilerException >{ exception };
		}
		catch ( ... )
		{
			// Free created objects
			Compiler::GarbageCollect( std::vector<QScript::FunctionObject*>{ } );

			// Free compilation materials (also frees main chunk)
			assembler.Release();

			// Free AST
			for ( auto node : astNodes )
			{
				node->Release();
				delete node;
			}
			astNodes.clear();

			// Rethrow
			throw;
		}
	}

	std::vector< Compiler::Type_t* > Typer( const std::string& source, const Config_t& config )
	{
		BEGIN_COMPILER;

		// Make sure module system is initialized
		QScript::InitModules();

		Chunk_t* chunk = AllocChunk();
		Compiler::Assembler assembler( chunk, config );

		// Import main system module (functions like exit(), print(), etc)
		auto systemModule = QScript::ResolveModule( "System" );
		systemModule->Import( &assembler );

		std::vector< Compiler::BaseNode* > astNodes;
		std::vector< Compiler::Type_t* > exprTypes;

		try
		{
			// Lexical analysis (tokenization)
			auto tokens = Compiler::Lexer( source );

			// Generate IR
			astNodes = Compiler::GenerateIR( tokens );

			// Remove last return node
			delete astNodes.back();
			astNodes.erase( astNodes.end() - 1 );

			for ( auto node : astNodes )
			{
				node->Compile( assembler );

				auto exprType = node->ExprType( assembler );
				exprTypes.push_back( Compiler::DeepCopyType( *exprType, QS_NEW Compiler::Type_t ) );

				// Attempt to resolve return type
				//if ( exprType->IsPrimitive( Compiler::TYPE_FUNCTION ) || exprType->IsPrimitive( Compiler::TYPE_NATIVE ) )
				//{
				//	switch ( node->Id() )
				//	{
				//	case Compiler::NODE_NAME:
				//	{
				//		auto nameValue = static_cast< Compiler::ValueNode* >( node )->GetValue();
				//		auto name = AS_STRING( nameValue )->GetString();

				//		Compiler::Variable_t varInfo;

				//		if ( assembler.FindGlobal( name, &varInfo ) )
				//			retnType = varInfo.m_Type->m_ReturnType ? *varInfo.m_Type->m_ReturnType : Compiler::Type_t( Compiler::TYPE_NONE );

				//		break;
				//	}
				//	case Compiler::NODE_FUNC:
				//	{
				//		auto funcNode = static_cast< Compiler::ListNode* >( node );
				//		Compiler::ResolveReturnType( funcNode, assembler, retnType );
				//		break;
				//	}
				//	case Compiler::NODE_CONSTVAR:
				//	case Compiler::NODE_VAR:
				//	{
				//		auto varNode = static_cast< Compiler::ListNode* >( node );
				//		auto valueNode = varNode->GetList()[ 1 ];

				//		if ( valueNode && valueNode->Id() == Compiler::NODE_FUNC )
				//		{
				//			auto funcNode = static_cast< Compiler::ListNode* >( valueNode );
				//			retnType = Compiler::ResolveReturnType( funcNode, assembler );
				//		}
				//		break;
				//	}
				//	case Compiler::NODE_ASSIGN:
				//	{
				//		auto assignNode = static_cast< Compiler::ComplexNode* >( node );
				//		auto valueNode = assignNode->GetRight();

				//		if ( valueNode && valueNode->Id() == Compiler::NODE_FUNC )
				//		{
				//			auto funcNode = static_cast< const Compiler::ListNode* >( valueNode );
				//			retnType = Type::ResolveReturnType( funcNode, assembler );
				//		}
				//		break;
				//	}
				//	default:
				//		break;
				//	}
				//}
			}

			for ( auto node : astNodes )
			{
				node->Release();
				delete node;
			}

			astNodes.clear();

			auto functions = assembler.Finish();

			// Clean up objects created in compilation process
			Compiler::GarbageCollect( functions );

			// Reset allocators
			END_COMPILER;

			// Free compiled function
			FreeFunction( functions.back() );

			return exprTypes;
		}
		catch ( const CompilerException& exception )
		{
			// Free created objects
			Compiler::GarbageCollect( std::vector<QScript::FunctionObject*>{ } );

			// Free compilation materials (also frees main chunk)
			assembler.Release();

			// Free AST
			for ( auto node : astNodes )
			{
				node->Release();
				delete node;
			}
			astNodes.clear();

			// Rethrow
			throw std::vector< CompilerException >{ exception };
		}
		catch ( ... )
		{
			// Free created objects
			Compiler::GarbageCollect( std::vector<QScript::FunctionObject*>{ } );

			// Free compilation materials (also frees main chunk)
			assembler.Release();

			// Free AST
			for ( auto node : astNodes )
			{
				node->Release();
				delete node;
			}
			astNodes.clear();

			// Rethrow
			throw;
		}
	}

	std::vector< Compiler::BaseNode* > GenerateAST( const std::string& source )
	{
		BEGIN_COMPILER;

		// Make sure module system is initialized
		QScript::InitModules();

		Chunk_t* chunk = AllocChunk();
		Compiler::Assembler assembler( chunk, QScript::Config_t( true ) );

		// Import main system module (functions like exit(), print(), etc)
		auto systemModule = QScript::ResolveModule( "System" );
		systemModule->Import( &assembler );

		std::vector< Compiler::BaseNode* > astNodes;

		try
		{
			// Lexical analysis (tokenization)
			auto tokens = Compiler::Lexer( source );

			// Generate IR
			astNodes = Compiler::GenerateIR( tokens );

			// Clean up objects created in compilation process
			Compiler::GarbageCollect( astNodes );

			// Reset allocators
			END_COMPILER;
			return astNodes;
		}
		catch ( const CompilerException& exception )
		{
			// Free created objects
			Compiler::GarbageCollect( std::vector<QScript::FunctionObject*>{ } );

			// Free compilation materials (also frees main chunk)
			assembler.Release();

			// Free AST
			for ( auto node : astNodes )
			{
				node->Release();
				delete node;
			}
			astNodes.clear();

			// Rethrow
			throw std::vector< CompilerException >{ exception };
		}
		catch ( ... )
		{
			// Free created objects
			Compiler::GarbageCollect( std::vector<QScript::FunctionObject*>{ } );

			// Free compilation materials (also frees main chunk)
			assembler.Release();

			// Free AST
			for ( auto node : astNodes )
			{
				node->Release();
				delete node;
			}

			astNodes.clear();

			// Rethrow
			throw;
		}
	}
}

namespace Compiler
{
	// List of allocated objects for garbage collection
	std::vector< QScript::Object* > ObjectList;

	uint32_t AddConstant( const QScript::Value& value, QScript::Chunk_t* chunk )
	{
		if ( IS_STRING( value ) )
		{
			// De-duplicate strings
			for ( uint32_t i = 0; i < chunk->m_Constants.size(); ++i )
			{
				if ( !IS_STRING( chunk->m_Constants[ i ] ) )
					continue;

				if ( AS_STRING( value )->GetString() == AS_STRING( chunk->m_Constants[ i ] )->GetString() )
					return i;
			}
		}
		else
		{
			// De-duplicate normal values
			for ( uint32_t i = 0; i < chunk->m_Constants.size(); ++i )
			{
				if ( ( chunk->m_Constants[ i ] == value ).IsTruthy() )
					return i;
			}
		}

		chunk->m_Constants.push_back( value );
		return ( uint32_t ) chunk->m_Constants.size() - 1;
	}

	void EmitByte( uint8_t byte, QScript::Chunk_t* chunk )
	{
		chunk->m_Code.push_back( byte );
	}

	QScript::StringObject* AllocateString( const std::string& string )
	{
		// Compiler object allocation must use pure 'new'
		// keyword, since FreeChunk() is also responsible for releasing these objects

		auto stringObject = QS_NEW QScript::StringObject( string );
		ObjectList.push_back( ( QScript::Object* ) stringObject );
		return stringObject;
	}

	QScript::FunctionObject* AllocateFunction( const std::string& name )
	{
		auto functionObject = QS_NEW QScript::FunctionObject( name, NULL );
		ObjectList.push_back( ( QScript::Object* ) functionObject );
		return functionObject;
	}

	QScript::NativeFunctionObject* AllocateNative( void* nativeFn )
	{
		assert( 0 );

		auto nativeObject = QS_NEW QScript::NativeFunctionObject( ( QScript::NativeFn ) nativeFn );
		ObjectList.push_back( ( QScript::Object* ) nativeObject );
		return nativeObject;
	}

	QScript::ClosureObject* AllocateClosure( QScript::FunctionObject* function )
	{
		assert( 0 );

		auto closureObject = QS_NEW QScript::ClosureObject( function );
		ObjectList.push_back( ( QScript::Object* ) closureObject );
		return closureObject;
	}

	QScript::UpvalueObject* AllocateUpvalue( QScript::Value* valuePtr )
	{
		assert( 0 );

		auto upvalueObject = QS_NEW QScript::UpvalueObject( valuePtr );
		ObjectList.push_back( ( QScript::Object* ) upvalueObject );
		return upvalueObject;
	}

	QScript::TableObject* AllocateTable( const std::string& name )
	{
		assert( 0 );

		auto tableObject = QS_NEW QScript::TableObject( name );
		ObjectList.push_back( ( QScript::Object* ) tableObject );
		return tableObject;
	}

	QScript::ArrayObject* AllocateArray( const std::string& name )
	{
		assert( 0 );

		auto arrayObject = QS_NEW QScript::ArrayObject( name );
		ObjectList.push_back( ( QScript::Object* ) arrayObject );
		return arrayObject;
	}

	void GarbageCollect( const std::vector< QScript::FunctionObject* >& functions )
	{
		std::vector< QScript::Value > values;
		for ( auto function : functions )
		{
			for ( auto value : function->GetChunk()->m_Constants )
			{
				if ( IS_OBJECT( value ) )
					values.push_back( value );
			}
		}

		GarbageCollect( values );
	}

	void GarbageCollect( const std::vector< Compiler::BaseNode* >& nodes )
	{
		std::vector< QScript::Value > values;
		std::vector< Compiler::BaseNode* > queue;

		for ( auto node : nodes )
			queue.push_back( node );

		while ( queue.size() > 0 )
		{
			auto node = queue[ 0 ];

			if ( node )
			{
				switch( node->Type() )
				{
					case NT_TERM:
						break;
					case NT_VALUE:
					{
						auto value = static_cast< const ValueNode* >( node )->GetValue();
						if ( IS_OBJECT( value ) )
							values.push_back( value );
						break;
					}
					case NT_SIMPLE:
						queue.push_back( static_cast< SimpleNode* >( node )->GetNode() );
						break;
					case NT_COMPLEX:
					{
						queue.push_back( static_cast< ComplexNode* >( node )->GetLeft() );
						queue.push_back( static_cast< ComplexNode* >( node )->GetRight() );
						break;
					}
					case NT_LIST:
					{
						auto list = static_cast< const ListNode* >( node )->GetList();

						for ( auto listNode : list )
							queue.push_back( listNode );

						break;
					}
					default:
						break;
				}
			}

			queue.erase( queue.begin() );
		}

		GarbageCollect( values );
	}

	void GarbageCollect( const std::vector< QScript::Value >& values )
	{
		for ( auto object : ObjectList )
		{
			bool isReferenced = false;

			for ( auto value : values )
			{
				if ( IS_OBJECT( value ) && AS_OBJECT( value ) == object )
					isReferenced = true;
			}

			if ( !isReferenced )
				delete object;
		}

		ObjectList.clear();
	}

	Assembler::Assembler( QScript::Chunk_t* chunk, const QScript::Config_t& config )
		: m_Config( config )
	{
		// Fill out globals for REPL
		for ( auto identifier : config.m_Globals )
			AddGlobal( identifier, false, -1, -1, &TA_UNKNOWN, NULL );

		CreateFunction( "<main>", true, &TA_UNKNOWN, true, true, chunk );
	}

	void Assembler::Release()
	{
		// Called when a compilation error occurred and all previously compiled
		// materials need to be freed

		for ( auto type : m_Types )
			FreeTypes( type, __FILE__, __LINE__ );

		for ( auto context : m_Functions )
		{
			delete context.m_Func->GetChunk();
			delete context.m_Func;
			delete context.m_Stack;
		}
	}

	std::vector< QScript::FunctionObject* > Assembler::Finish()
	{
		if ( m_Functions.size() != 1 )
			throw Exception( "cp_unescaped_function", "Abort: Compiler was left with unfinished functions" );

		delete CurrentStack();

		for ( auto type : m_Types )
			FreeTypes( type, __FILE__, __LINE__ );

		m_Compiled.push_back( m_Functions[ 0 ].m_Func );
		m_Functions.pop_back();

		return m_Compiled;
	}

	const std::vector< Variable_t >& Assembler::CurrentArguments()
	{
		return m_FunctionArgs;
	}

	QScript::Chunk_t* Assembler::CurrentChunk()
	{
		return m_Functions.back().m_Func->GetChunk();
	}

	const Assembler::FunctionContext_t* Assembler::CurrentContext()
	{
		return &m_Functions.back();
	}

	QScript::FunctionObject* Assembler::CurrentFunction()
	{
		return m_Functions.back().m_Func;
	}

	Assembler::Stack_t* Assembler::CurrentStack()
	{
		return m_Functions.back().m_Stack;
	}

	QScript::FunctionObject* Assembler::CreateFunction( const std::string& name, bool isConst, const Type_t* type, bool isAnonymous, bool addLocal, QScript::Chunk_t* chunk )
	{
		auto function = QS_NEW QScript::FunctionObject( name, chunk );
		auto context = FunctionContext_t{ function, QS_NEW Assembler::Stack_t(), DeepCopyType( *type, RegisterType( QS_NEW Type_t, __FILE__, __LINE__ ) ) };

		m_Functions.push_back( context );

		if ( addLocal )
			AddLocal( isAnonymous ? "" : name, isConst, -1, -1, type );

		return function;
	}

	void Assembler::FinishFunction( int lineNr, int colNr, std::vector< Upvalue_t >* upvalues )
	{
		auto function = &m_Functions.back();
		*upvalues = function->m_Upvalues;

		// Implicit return (null)
		EmitByte( QScript::OpCode::OP_LOAD_NULL, function->m_Func->GetChunk() );
		EmitByte( QScript::OpCode::OP_RETURN, function->m_Func->GetChunk() );

		auto& locals = CurrentStack()->m_Locals;

		if ( m_Config.m_IdentifierCb && locals.size() > 0 )
		{
			if ( locals[ 0 ].m_Var.m_Function )
				m_Config.m_IdentifierCb( lineNr, colNr, locals[ 0 ].m_Var, "Function" );
		}

		// Finished compiling
		m_Compiled.push_back( function->m_Func );

		// Free compile-time stack & types from memory
		// FreeTypes( CurrentContext()->m_Type );
		delete CurrentStack();

		m_Functions.pop_back();
	}

	void Assembler::AddArgument( const std::string& name, bool isConstant, int lineNr, int colNr, const Type_t* type )
	{
		auto variable = Variable_t( name, isConstant, DeepCopyType( *type, RegisterType( QS_NEW Type_t, __FILE__, __LINE__ ) ) );

		if ( m_Config.m_IdentifierCb )
			m_Config.m_IdentifierCb( lineNr, colNr, variable, "Argument" );

		m_FunctionArgs.push_back( variable );
	}

	uint32_t Assembler::AddLocal( const std::string& name, bool isConstant, int lineNr, int colNr, const Type_t* type, QScript::FunctionObject* fn )
	{
		auto stack = CurrentStack();

		auto variable = Variable_t( name, isConstant, DeepCopyType( *type, RegisterType( QS_NEW Type_t, __FILE__, __LINE__ ) ), fn );

		if ( m_Config.m_IdentifierCb )
			m_Config.m_IdentifierCb( lineNr, colNr, variable, "Local" );

		stack->m_Locals.push_back( Local_t{ variable, stack->m_CurrentDepth, false } );
		return ( uint32_t ) stack->m_Locals.size() - 1;
	}

	Local_t* Assembler::GetLocal( int local )
	{
		return &CurrentStack()->m_Locals[ local ];
	}

	bool Assembler::FindLocalFromStack( Stack_t* stack, const std::string& name, uint32_t* out, Variable_t* varInfo )
	{
		for ( int i = ( int ) stack->m_Locals.size() - 1; i >= 0 ; --i )
		{
			if ( stack->m_Locals[ i ].m_Var.m_Name == name )
			{
				*out = ( uint32_t ) i;
				*varInfo = stack->m_Locals[ i ].m_Var;
				return true;
			}
		}

		return false;
	}

	bool Assembler::FindArgument( const std::string& name, Variable_t* out )
	{
		for ( auto arg : m_FunctionArgs )
		{
			if ( arg.m_Name == name )
			{
				*out = arg;
				return true;
			}
		}

		return false;
	}

	bool Assembler::FindGlobal( const std::string& name, Variable_t* out )
	{
		auto global = m_Globals.find( name );

		if ( global == m_Globals.end() )
			return false;

		if ( out )
			*out = global->second;

		return true;
	}

	bool Assembler::FindLocal( const std::string& name, uint32_t* out, Variable_t* varInfo )
	{
		return FindLocalFromStack( CurrentStack(), name, out, varInfo );
	}

	bool Assembler::FindUpvalue( const std::string name, uint32_t* out, Variable_t* varInfo )
	{
		int thisFunction = ( int ) m_Functions.size() - 1;
		for ( int i = thisFunction; i > 0; --i )
		{
			uint32_t upValue = 0;
			if ( !FindLocalFromStack( m_Functions[ i - 1 ].m_Stack, name, &upValue, varInfo ) )
				continue;

			*out = upValue;
			return true;
		}

		return false;
	}

	bool Assembler::AddGlobal( const std::string& name, bool isConstant, int lineNr, int colNr, const Type_t* type, QScript::FunctionObject* fn )
	{
		if ( m_Globals.find( name ) != m_Globals.end() )
			return false;

		Variable_t global = Variable_t( name, isConstant, DeepCopyType( *type, RegisterType( QS_NEW Type_t, __FILE__, __LINE__ ) ), fn );
		m_Globals.insert( std::make_pair( name, global ) );

		if ( m_Config.m_IdentifierCb )
			m_Config.m_IdentifierCb( lineNr, colNr, global, "Global" );

		return true;
	}

	bool Assembler::RequestUpvalue( const std::string name, uint32_t* out, int lineNr, int colNr, Variable_t* varInfo )
	{
		int thisFunction = ( int ) m_Functions.size() - 1;

		for ( int i = thisFunction; i > 0; --i )
		{
			uint32_t upValue = 0;
			if ( !FindLocalFromStack( m_Functions[ i - 1 ].m_Stack, name, &upValue, varInfo ) )
				continue;

			// Capture it
			m_Functions[ i - 1 ].m_Stack->m_Locals[ upValue ].m_Captured = true;

			// Link upvalue through the closure chain
			for ( int j = i; j <= thisFunction; ++j )
				upValue = AddUpvalue( &m_Functions[ j ], upValue, j == i, lineNr, colNr, varInfo );

			*out = upValue;
			return true;
		}

		return false;
	}

	uint32_t Assembler::AddUpvalue( FunctionContext_t* context, uint32_t index, bool isLocal, int lineNr, int colNr, Variable_t* varInfo )
	{
		for ( uint32_t i = 0; i < context->m_Upvalues.size(); ++i )
		{
			auto upvalue = context->m_Upvalues[ i ];
			if ( upvalue.m_Index == index && upvalue.m_IsLocal == isLocal )
				return i;
		}

		if ( m_Config.m_IdentifierCb )
			m_Config.m_IdentifierCb( lineNr, colNr, *varInfo, "Upvalue" );

		context->m_Upvalues.push_back( Upvalue_t{ isLocal, index } );

		uint32_t numUpvalues = ( uint32_t ) context->m_Upvalues.size();

		context->m_Func->SetUpvalues( ( int ) numUpvalues );
		return numUpvalues - 1;
	}

	int Assembler::StackDepth()
	{
		return CurrentStack()->m_CurrentDepth;
	}

	bool Assembler::IsTopLevel()
	{
		return m_Functions.size() == 1;
	}

	void Assembler::PushScope()
	{
		++CurrentStack()->m_CurrentDepth;
	}

	void Assembler::PopScope()
	{
		auto stack = CurrentStack();

		for ( int i = ( int ) stack->m_Locals.size() - 1; i >= 0; --i )
		{
			auto local = stack->m_Locals[ i ];

			if ( local.m_Depth < stack->m_CurrentDepth )
				break;

			if ( !local.m_Captured )
				EmitByte( QScript::OpCode::OP_POP, CurrentChunk() );
			else
				EmitByte( QScript::OpCode::OP_CLOSE_UPVALUE, CurrentChunk() );

			// Free types
			// FreeTypes( local.m_Var.m_Type );

			stack->m_Locals.erase( stack->m_Locals.begin() + i );
		}

		--stack->m_CurrentDepth;
	}

	Type_t* Assembler::RegisterType( Type_t* type, const char* file, int line )
	{
		if ( std::find( m_Types.begin(), m_Types.end(), type ) != m_Types.end() ) {
			throw std::exception( "Type already exists in registered types. There is a programming error." );
		}

		// printf( "type %X registerted from %s line %i\n", type, file, line );

		m_Types.push_back( type );
		return type;
	}

	const Type_t* Assembler::UnregisterType( const Type_t* type )
	{
		if ( !type )
			return NULL;

		UnregisterType( type->m_ReturnType );

		for ( auto arg : type->m_ArgTypes )
			UnregisterType( arg.second );

		for ( auto prop : type->m_PropTypes )
			UnregisterType( prop.second );

		for ( auto indice : type->m_IndiceTypes )
			UnregisterType( indice );

		m_Types.erase( std::remove( m_Types.begin(), m_Types.end(), type ), m_Types.end() );

		return type;
	}

	const QScript::Config_t& Assembler::Config() const
	{
		return m_Config;
	}

	void Assembler::ClearArguments()
	{
		//for ( auto arg : m_FunctionArgs )
		//	FreeTypes( arg.m_Type );

		m_FunctionArgs.clear();
	}
}
