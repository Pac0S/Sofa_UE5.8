#pragma once

#include "CoreMinimal.h"

#ifdef check
#undef check
#endif

#ifdef TEXT
#undef TEXT
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4250)
#endif

THIRD_PARTY_INCLUDES_START

#include <sofa/simulation/config.h>

// Neutraliser le macro d'export pour éviter les conflits avec UE
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

THIRD_PARTY_INCLUDES_END

#ifdef _MSC_VER
#pragma warning(pop)
#endif