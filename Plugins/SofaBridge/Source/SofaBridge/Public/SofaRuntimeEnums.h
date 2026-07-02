#pragma once

#include "CoreMinimal.h"
#include "SofaRuntimeEnums.generated.h"

UENUM(BlueprintType)
enum class ESofaRuntimeObjectRole : uint8
{
    Unknown UMETA(DisplayName = "Unknown"),
    DeformableLayer UMETA(DisplayName = "Deformable Layer"),
    Organ UMETA(DisplayName = "Organ"),
    Tool UMETA(DisplayName = "Tool"),
    StaticSupport UMETA(DisplayName = "Static Support"),
    CollisionProxy UMETA(DisplayName = "Collision Proxy")
};