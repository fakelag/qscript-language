#include "QLibPCH.h"
#include "NativeModule.h"

// Modules
#include "System.h"
#include "Time.h"

#define NATIVE_MODULE( moduleClass ) \
static moduleClass __##moduleClass; \
NativeModules.push_back( &__##moduleClass )


std::vector< QScript::NativeModule* > NativeModules;

namespace QScript
{
	const NativeModule* ResolveModule( const std::string& name )
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

		NATIVE_MODULE( SystemModule );
		NATIVE_MODULE( TimeModule );
	}
}
