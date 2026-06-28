#pragma once

#include "CoreMinimal.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4250) // 'class1' : inherits 'class2::member' via dominance
#pragma warning(disable : 4996) // 'function': was declared deprecated
#pragma warning(disable : 4005) // 'macro' : macro redefinition
#pragma warning(disable : 4263) // 'function' : member function does not override any base class virtual member function
#pragma warning(disable : 4264) // 'function' : no override available for virtual member function from base 'type'; function is hidden

#endif

THIRD_PARTY_INCLUDES_START

#pragma push_macro("check")
#pragma push_macro("verify")
#pragma push_macro("ensure")
#pragma push_macro("PI")
#pragma push_macro("TEXT")
#pragma push_macro("CONSTEXPR")
#pragma push_macro("dynamic_cast")
#pragma push_macro("UNICODE")
#undef check
#undef verify
#undef ensure
#undef TEXT
#undef PI
#undef CONSTEXPR
#undef UNICODE


// Temporary workaround:
// In Unreal's compilation environment, SOFA_SIMULATION_CORE_API expands
// incorrectly for this TU and breaks declarations in sofa/simulation/fwd.h.
// We neutralize it locally to keep integration moving.
// TODO: replace with a Build.cs / import-export clean solution.
#ifdef SOFA_SIMULATION_CORE_API
#undef SOFA_SIMULATION_CORE_API
#endif
#define SOFA_SIMULATION_CORE_API

#include <sofa/config.h>
#include <sofa/simulation/config.h>
#include <sofa/simulation/Node.h>

#include <sofa/simpleapi/SimpleApi.h>
#include <sofa/simulation/Simulation.h>
#include <sofa/simulation/common/init.h>
#include <sofa/simulation/graph/init.h>
#include <sofa/simulation/fwd.h>
#include <sofa/simulation/graph/DAGSimulation.h>

#include <sofa/core/objectmodel/SPtr.h>
#include <sofa/core/objectmodel/BaseObject.h>
#include <sofa/core/objectmodel/BaseData.h>
#include <sofa/core/ObjectFactory.h>

#include <sofa/helper/system/PluginManager.h>
#include <sofa/helper/system/FileRepository.h>

#include "sofa/defaulttype/VecTypes.h"

#include <sofa/component/odesolver/backward/EulerImplicitSolver.h>
#include <sofa/component/statecontainer/MechanicalObject.h>
#include <sofa/component/mass/UniformMass.h>
#include <sofa/component/solidmechanics/fem/elastic/TetrahedronFEMForceField.h>
#include <sofa/component/io/mesh/MeshGmshLoader.h>

#pragma pop_macro("UNICODE")
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