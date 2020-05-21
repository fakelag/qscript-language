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
		IrBuilder_t*					CurrentBuilder()					const { CheckEOF(); return m_Builders[ m_CurrentBuilder ]; }
		IrBuilder_t*					NextBuilder() 						{ CheckEOF(); return m_Builders[ m_CurrentBuilder++ ]; }
		int 							Offset()							{ return m_CurrentBuilder; }

		IrBuilder_t*					Peek( int offset )
		{
			if ( offset < 0 || offset > ( int ) m_Builders.size() - 1 )
				return NULL;

			return m_Builders[ offset ];
		}

		void 							Expect( Token token, const std::string desc )
		{
			CheckEOF();

			auto builder = CurrentBuilder();
			if ( builder->m_Token.m_Id != token )
			{
				throw CompilerException( "ir_expect", desc, builder->m_Token.m_LineNr,
					builder->m_Token.m_ColNr, builder->m_Token.m_String );
			}

			NextBuilder();
		}

		bool 							Match( Token token )
		{
			CheckEOF();

			if ( ( size_t ) m_CurrentBuilder + 1 >= m_Builders.size() )
				return false;

			if ( m_Builders[ m_CurrentBuilder + 1 ]->m_Token.m_Id == token )
			{
				NextBuilder();
				return true;
			}

			return false;
		}

		bool 							MatchCurrent( Token token )
		{
			CheckEOF();

			if ( m_Builders[ m_CurrentBuilder ]->m_Token.m_Id == token )
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
				switch ( m_Builders[ m_CurrentBuilder ]->m_Token.m_Id )
				{
				case Compiler::TOK_SCOLON:
					++m_CurrentBuilder;
				case Compiler::TOK_BRACE_RIGHT:
					break;
				default:
					++m_CurrentBuilder;
					continue;
				}

				break;
			}
		}

		BaseNode*						ToScope( BaseNode* node )
		{
			if ( node->Id() != NODE_SCOPE )
			{
				return AllocateNode< ListNode >( node->LineNr(), node->ColNr(), node->Token(),
					NODE_SCOPE, std::vector< BaseNode* >{ node } );
			}

			return node;
		}

		void							CheckEOF() const
		{
			if ( !IsFinished() )
				return;

			int lineNr, colNr;
			std::string token;

			if ( m_Builders.size() > 0 )
			{
				auto lastToken = m_Builders[ m_Builders.size() - 1 ];

				lineNr = lastToken->m_Token.m_LineNr;
				colNr = lastToken->m_Token.m_ColNr;
				token = lastToken->m_Token.m_String;
			}

			throw CompilerException( "ir_parsing_past_eof", "Parsing past end of file", lineNr,
				colNr, token );
		}

		template <typename T, class... Args>
		T* 								AllocateNode(Args&& ... args)
		{
			static_assert( std::is_base_of<BaseNode, T>::value, "Allocated node must derive from BaseNode" );

			auto node = QS_NEW T( args... );
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
