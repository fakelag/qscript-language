#include "QLibPCH.h"
#include "NativeModule.h"

std::vector< QScript::QNativeModule* > NativeModules;

namespace QScript
{
	const QNativeModule* ResolveModule( const std::string& name )
	{
		for ( auto module : NativeModules )
		{
			if ( module->GetName() == name )
				return module;
		}

		return NULL;
	}

	void RegisterModule( QNativeModule* module )
	{
		NativeModules.push_back( module );
	}
}
