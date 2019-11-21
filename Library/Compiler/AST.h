namespace Compiler
{
	class IASTNode
	{
	public:
		virtual ~IASTNode();
		virtual void Compile() = 0;
	};

	class SimpleNode
	{
	public:
		void Compile();
	};
}
