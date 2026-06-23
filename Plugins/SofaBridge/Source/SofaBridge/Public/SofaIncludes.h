#pragma once

#include "CoreMinimal.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4250)
#pragma warning(disable : 4996)
#endif

THIRD_PARTY_INCLUDES_START

#pragma push_macro("check")
#pragma push_macro("verify")
#pragma push_macro("ensure")
#pragma push_macro("PI")
#pragma push_macro("TEXT")
#pragma push_macro("CONSTEXPR")
#pragma push_macro("dynamic_cast")
#undef check
#undef verify
#undef ensure
#undef TEXT
#undef PI
#undef CONSTEXPR
#undef dynamic_cast

#include <sofa/simulation/config.h>

#ifdef SOFA_SIMULATION_CORE_API
#undef SOFA_SIMULATION_CORE_API
#endif
#define SOFA_SIMULATION_CORE_API

#include <sofa/simulation/Simulation.h>
#include <sofa/simpleapi/SimpleApi.h>
#include <sofa/simulation/common/init.h>
#include <sofa/simulation/graph/init.h>
#include <sofa/simulation/fwd.h>
#include <sofa/simulation/Node.h>
#include <sofa/simulation/graph/DAGSimulation.h>

#pragma pop_macro("dynamic_cast")
#pragma pop_macro("CONSTEXPR")
#pragma pop_macro("PI")
#pragma pop_macro("TEXT")
#pragma pop_macro("ensure")
#pragma pop_macro("verify")
#pragma pop_macro("check")

THIRD_PARTY_INCLUDES_END

#ifdef _MSC_VER
#pragma warning(pop)
#endif