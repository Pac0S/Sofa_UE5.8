#include "SofaTestComponent.h"
#include "SofaIncludes.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogSofaTest, Log, All);

USofaTestComponent::USofaTestComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USofaTestComponent::BeginPlay()
{
    Super::BeginPlay();

    // Lancer un test au démarrage (tu peux commenter si tu préfères appeler manuellement)
    RunSofaTest();
}

void USofaTestComponent::RunSofaTest()
{
#if !SOFA_SDK_ENABLED
    UE_LOG(LogSofaTest, Warning, TEXT("SOFA SDK not enabled, skipping test"));
    return;
#endif

    UE_LOG(LogSofaTest, Log, TEXT("=== SOFA Test: SimpleApi + Simulation ==="));

    using sofa::simulation::Simulation;
    using sofa::simulation::NodeSPtr;

    // 1) Init SOFA (comme fallingSOFA, mais sans GUI)
    sofa::simulation::common::init();
    sofa::simulation::graph::init();

    // 2) Test namespace sofa::simulation::graph (compilation)
    {
        // On n'instancie rien ici, on vérifie que le type existe
        // Si DAGSimulation.h déclare bien namespace sofa::simulation::graph, cette ligne compile
        // (si tu veux, tu peux décommenter pour forcer l'utilisation)
        sofa::simulation::graph::DAGSimulation* TmpSim = nullptr;
        (void)TmpSim;
    }

    // 3) Simulation::SPtr via SimpleApi
    Simulation::SPtr Simu = sofa::simpleapi::createSimulation("DAG");
    if (!Simu)
    {
        UE_LOG(LogSofaTest, Error, TEXT("Failed to create SOFA Simulation via SimpleApi"));
        return;
    }
    UE_LOG(LogSofaTest, Log, TEXT("SOFA Simulation created"));

    // 4) NodeSPtr / Root node
    NodeSPtr Root = sofa::simpleapi::createRootNode(Simu, "root");
    if (!Root)
    {
        UE_LOG(LogSofaTest, Error, TEXT("Failed to create SOFA root node via SimpleApi"));
        return;
    }
    UE_LOG(LogSofaTest, Log, TEXT("SOFA Root node created"));

    // 5) Quelques appels simples sur Root pour vérifier que l'API fonctionne
    Root->setName("UE5Root");
    Root->setAnimate(true);
    Root->setDt(0.01);

    Root->setGravity(sofa::type::Vec3(0.0, -9.81, 0.0));

    UE_LOG(LogSofaTest, Log, TEXT("SOFA Root configured (gravity, dt, name)"));

    // 6) Un petit step de simulation à vide
    // Suivant l'API de Simulation, adapte la signature (root ou non)
    sofa::simulation::node::animate(Root.get(), 0.01);

    UE_LOG(LogSofaTest, Log, TEXT("SOFA animate() called successfully"));

    // 7) Clean minimal
    // Tu peux ajouter ici un unload si nécessaire
    //Simu->unload(Root.get());

    UE_LOG(LogSofaTest, Log, TEXT("=== SOFA Test completed ==="));
}