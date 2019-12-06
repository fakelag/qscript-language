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
			m_CurrentObject = 0;
		}

		~ParserState()
		{
			for ( auto builder : m_Builders )
				delete builder;

			m_Builders.clear();
		}

		std::vector< BaseNode* >& 		Product() { return m_Ast; }
		std::vector< IrBuilder_t* >& 	Builders() { return m_Builders; }

		bool 							IsFinished() const { return ( size_t ) m_CurrentObject >= m_Builders.size(); }
		IrBuilder_t*					CurrentBuilder() const { return m_Builders[ m_CurrentObject ]; }
		IrBuilder_t*					NextBuilder() { return m_Builders[ m_CurrentObject++ ]; }

		void 							AddNode( BaseNode* node ) { m_Ast.push_back( node ); }

	private:
		int 						m_CurrentObject;
		std::vector< BaseNode* > 	m_Ast;
		std::vector< IrBuilder_t* > m_Builders;
	};
}
