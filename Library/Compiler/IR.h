#pragma once

namespace Compiler
{
	struct IrBuilder_t;

	using LedFn = std::function<BaseNode*( const IrBuilder_t& irBuilder, BaseNode* left )>;
	using NudFn = std::function<BaseNode*( const IrBuilder_t& irBuilder )>;

	struct IrBuilder_t
	{
		IrBuilder_t( const Token_t& token )
		{
			m_Token = token;
			m_Led = NULL;
			m_Nud = NULL;
		}

		Token_t 		m_Token;
		LedFn			m_Led;
		NudFn			m_Nud;
	};

	class ParserState
	{
	public:
		ParserState()
		{
			m_CurrentBuilder = 0;
		}

		~ParserState()
		{
			for ( auto builder : m_Builders )
				delete builder;

			if ( m_Errors.size() > 0 )
			{
				// A parsing error occurred, free all allocated nodes
				for ( auto node : m_AllocatedNodes )
					delete node;

				m_AllocatedNodes.clear();
			}

			m_Builders.clear();
		}

		void 								AddNode( BaseNode* node )	{ m_Ast.push_back( node ); }
		std::vector< BaseNode* >& 			Product()					{ return m_Ast; }
		std::vector< IrBuilder_t* >& 		Builders()					{ return m_Builders; }
		std::vector< CompilerException >&	Errors()					{ return m_Errors; }

		bool 							IsFinished()						const { return ( size_t ) m_CurrentBuilder >= m_Builders.size(); }
		bool 							IsError()							const { return m_Errors.size() > 0; }
		IrBuilder_t*					CurrentBuilder()					const { return m_Builders[ m_CurrentBuilder ]; }
		IrBuilder_t*					NextBuilder() 						{ return m_Builders[ m_CurrentBuilder++ ]; }

		void 							Expect( Token token, const std::string desc )
		{
			auto builder = CurrentBuilder();
			if ( builder->m_Token.m_Token != token )
			{
				throw CompilerException( "ir_expect", desc, builder->m_Token.m_LineNr,
					builder->m_Token.m_ColNr, builder->m_Token.m_String );
			}

			NextBuilder();
		}

		bool 							Match( Token token )
		{
			if ( IsFinished() )
				return false;

			if ( m_Builders[ m_CurrentBuilder + 1 ]->m_Token.m_Token == token )
			{
				NextBuilder();
				return true;
			}
			
			return false;
		}

		void							AddErrorAndResync( const CompilerException& exception )
		{
			m_Errors.push_back( exception );

			while ( !IsFinished() )
			{
				switch ( m_Builders[ m_CurrentBuilder ]->m_Token.m_Token )
				{
				case Compiler::TOK_SCOLON:
				case Compiler::TOK_BRACE_RIGHT:
					++m_CurrentBuilder;
					break;
				default:
					++m_CurrentBuilder;
					continue;
				}

				break;
			}
		}

		template <typename T, class... Args>
		T* 								AllocateNode(Args&& ... args)
		{
			static_assert( std::is_base_of<BaseNode, T>::value, "Allocated node must derive from BaseNode" );

			auto node = new T( args... );
			m_AllocatedNodes.push_back( ( BaseNode* ) node );

			return node;
		}

	private:
		int 								m_CurrentBuilder;
		std::vector< BaseNode* > 			m_Ast;
		std::vector< IrBuilder_t* > 		m_Builders;

		std::vector< CompilerException >	m_Errors;
		std::vector< BaseNode* > 			m_AllocatedNodes;
	};
}
