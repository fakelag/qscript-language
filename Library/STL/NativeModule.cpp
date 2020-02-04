#include "QLibPCH.h"
#include "NativeModule.h"

// Modules
#include "System.h"
#include "Time.h"

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

	void InitModules()
	{
		if ( NativeModules.size() > 0 )
			return;

		NativeModules.push_back( new QSystem() );
		NativeModules.push_back( new QTime() );
	}
}
