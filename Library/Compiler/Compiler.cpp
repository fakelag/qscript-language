#include "QLibPCH.h"
#include "../Common/Chunk.h"
#include "../STL/NativeModule.h"

#include "Instructions.h"
#include "Compiler.h"

#define BEGIN_COMPILER \
Object::AllocateString = &Compiler::AllocateString; \
Object::AllocateFunction = &Compiler::AllocateFunction; \
Object::AllocateNative = &Compiler::AllocateNative; \
Object::AllocateClosure = &Compiler::AllocateClosure; \
Object::AllocateUpvalue = &Compiler::AllocateUpvalue; \
Object::AllocateClass = &Compiler::AllocateClass; \
Object::AllocateInstance = &Compiler::AllocateInstance

#define END_COMPILER \
Object::AllocateString = NULL; \
Object::AllocateFunction = NULL; \
Object::AllocateNative = NULL; \
Object::AllocateClosure = NULL; \
Object::AllocateUpvalue = NULL; \
Object::AllocateClass = NULL; \
Object::AllocateInstance = NULL

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

	std::vector< std::pair< uint32_t, uint32_t > > Typer( const std::string& source, const Config_t& config )
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
		std::vector< std::pair< uint32_t, uint32_t > > exprTypes;

		try
		{
			// Lexical analysis (tokenization)
			auto tokens = Compiler::Lexer( source );

			// Generate IR
			astNodes = Compiler::GenerateIR( tokens );

			// Remove last return node
			delete astNodes.back();
			astNodes.erase( astNodes.end() - 1 );

			// Compile bytecode
			for ( auto node : astNodes )
			{
				node->Compile( assembler );

				uint32_t exprType = node->ExprType( assembler );
				uint32_t retnType = Compiler::TYPE_NONE;

				// Attempt to resolve return type
				if ( exprType == Compiler::TYPE_FUNCTION || exprType == Compiler::TYPE_NATIVE )
				{
					switch ( node->Id() )
					{
					case Compiler::NODE_NAME:
					{
						auto nameValue = static_cast< Compiler::ValueNode* >( node )->GetValue();
						auto name = AS_STRING( nameValue )->GetString();

						Compiler::Assembler::Variable_t varInfo;

						if ( assembler.FindGlobal( name, &varInfo ) )
							retnType = varInfo.m_ReturnType;

						break;
					}
					case Compiler::NODE_FUNC:
					{
						auto funcNode = static_cast< Compiler::ListNode* >( node );
						retnType = Compiler::ResolveReturnType( funcNode, assembler );
						break;
					}
					case Compiler::NODE_CONSTVAR:
					case Compiler::NODE_VAR:
					{
						auto varNode = static_cast< Compiler::ListNode* >( node );
						auto valueNode = varNode->GetList()[ 1 ];

						if ( valueNode && valueNode->Id() == Compiler::NODE_FUNC )
						{
							auto funcNode = static_cast< Compiler::ListNode* >( valueNode );
							retnType = Compiler::ResolveReturnType( funcNode, assembler );
						}
						break;
					}
					case Compiler::NODE_ASSIGN:
					{
						auto assignNode = static_cast< Compiler::ComplexNode* >( node );
						auto valueNode = assignNode->GetRight();

						if ( valueNode && valueNode->Id() == Compiler::NODE_FUNC )
						{
							auto funcNode = static_cast< const Compiler::ListNode* >( valueNode );
							retnType = Compiler::ResolveReturnType( funcNode, assembler );
						}
						break;
					}
					default:
						break;
					}
				}

				exprTypes.push_back( std::make_pair( exprType, retnType ) );
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

	QScript::FunctionObject* AllocateFunction( const std::string& name, int arity )
	{
		auto functionObject = QS_NEW QScript::FunctionObject( name, arity, NULL );
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

	QScript::ClassObject* AllocateClass( const std::string& name )
	{
		assert( 0 );

		auto classObject = QS_NEW QScript::ClassObject( name );
		ObjectList.push_back( ( QScript::Object* ) classObject );
		return classObject;
	}

	QScript::InstanceObject* AllocateInstance( QScript::ClassObject* classDef )
	{
		assert( 0 );

		auto instObject = QS_NEW QScript::InstanceObject( classDef );
		ObjectList.push_back( ( QScript::Object* ) instObject );
		return instObject;
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
		std::vector< const Compiler::BaseNode* > queue;

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
						queue.push_back( static_cast< const SimpleNode* >( node )->GetNode() );
						break;
					case NT_COMPLEX:
					{
						queue.push_back( static_cast< const ComplexNode* >( node )->GetLeft() );
						queue.push_back( static_cast< const ComplexNode* >( node )->GetRight() );
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
		for ( auto identifier : config.m_Globals )
			AddGlobal( identifier );

		CreateFunction( "<main>", true, TYPE_UNKNOWN, 0, true, chunk );
	}

	void Assembler::Release()
	{
		// Called when a compilation error occurred and all previously compiled
		// materials need to be freed

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
		m_Compiled.push_back( m_Functions[ 0 ].m_Func );
		m_Functions.pop_back();

		return m_Compiled;
	}

	const std::vector< Assembler::Variable_t >& Assembler::CurrentArguments()
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

	QScript::FunctionObject* Assembler::CreateFunction( const std::string& name, bool isConst, uint32_t retnType, int arity, bool isAnonymous, QScript::Chunk_t* chunk )
	{
		auto function = QS_NEW QScript::FunctionObject( name, arity, chunk );
		auto context = FunctionContext_t{ function, QS_NEW Assembler::Stack_t(), retnType };

		m_Functions.push_back( context );

		AddLocal( isAnonymous ? "" : name, isConst, TYPE_FUNCTION, retnType );
		return function;
	}

	void Assembler::FinishFunction( QScript::FunctionObject** func, std::vector< Upvalue_t >* upvalues )
	{
		auto function = &m_Functions.back();

		*func = function->m_Func;
		*upvalues = function->m_Upvalues;

		// Implicit return (null)
		EmitByte( QScript::OpCode::OP_LOAD_NULL, function->m_Func->GetChunk() );
		EmitByte( QScript::OpCode::OP_RETURN, function->m_Func->GetChunk() );

		// Finished compiling
		m_Compiled.push_back( function->m_Func );

		// Free compile-time stack from memory
		delete CurrentStack();
		m_Functions.pop_back();
	}

	void Assembler::AddArgument( const std::string& name, bool isConstant, uint32_t type, uint32_t returnType )
	{
		m_FunctionArgs.push_back( Variable_t{ name, isConstant, type, returnType } );
	}

	uint32_t Assembler::AddLocal( const std::string& name )
	{
		return AddLocal( name, false, TYPE_UNKNOWN, TYPE_UNKNOWN );
	}

	uint32_t Assembler::AddLocal( const std::string& name, bool isConstant, uint32_t type, uint32_t returnType )
	{
		auto stack = CurrentStack();

		auto variable = Variable_t{ name, isConstant, type, returnType };

		stack->m_Locals.push_back( Assembler::Local_t{ variable, stack->m_CurrentDepth, false } );
		return ( uint32_t ) stack->m_Locals.size() - 1;
	}

	Assembler::Local_t* Assembler::GetLocal( int local )
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

	bool Assembler::RequestUpvalue( const std::string name, uint32_t* out, Variable_t* varInfo )
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
				upValue = AddUpvalue( &m_Functions[ j ], upValue, j == i );

			*out = upValue;
			return true;
		}

		return false;
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

	bool Assembler::AddGlobal( const std::string& name )
	{
		return AddGlobal( name, false, TYPE_UNKNOWN, TYPE_UNKNOWN );
	}

	bool Assembler::AddGlobal( const std::string& name, bool isConstant, uint32_t type, uint32_t returnType )
	{
		if ( m_Globals.find( name ) != m_Globals.end() )
			return false;

		Variable_t global = Variable_t{ name, isConstant, type, returnType };
		m_Globals.insert( std::make_pair( name, global ) );
		return true;
	}

	uint32_t Assembler::AddUpvalue( FunctionContext_t* context, uint32_t index, bool isLocal )
	{
		for ( uint32_t i = 0; i < context->m_Upvalues.size(); ++i )
		{
			auto upvalue = context->m_Upvalues[ i ];
			if ( upvalue.m_Index == index && upvalue.m_IsLocal == isLocal )
				return i;
		}

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

			stack->m_Locals.erase( stack->m_Locals.begin() + i );
		}

		--stack->m_CurrentDepth;
	}

	const QScript::Config_t& Assembler::Config() const
	{
		return m_Config;
	}

	void Assembler::ClearArguments()
	{
		m_FunctionArgs.clear();
	}
}
