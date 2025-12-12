// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattleFlowGuide.h"
#include "CattleAreaSubsystem.h"
#include "Components/SplineComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "DrawDebugHelpers.h"

ACattleFlowGuide::ACattleFlowGuide()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create root
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    // Create flow spline
    FlowSpline = CreateDefaultSubobject<USplineComponent>(TEXT("FlowSpline"));
    FlowSpline->SetupAttachment(SceneRoot);
    FlowSpline->SetClosedLoop(false);

    // Set up default spline points
    FlowSpline->ClearSplinePoints();
    FlowSpline->AddSplinePoint(FVector(-500.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local);
    FlowSpline->AddSplinePoint(FVector(500.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local);

    // Create influence box
    InfluenceBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InfluenceBox"));
    InfluenceBox->SetupAttachment(SceneRoot);
    InfluenceBox->SetBoxExtent(FVector(600.0f, 300.0f, 250.0f));
    InfluenceBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InfluenceBox->SetGenerateOverlapEvents(false);

#if WITH_EDITORONLY_DATA
    FlowSpline->SetDrawDebug(true);
#endif
}

void ACattleFlowGuide::BeginPlay()
{
    Super::BeginPlay();

    // Initialize flowmap if using painted mode
    if (bUsePaintedFlowmap)
    {
        InitializeFlowmap();
    }

    RegisterWithSubsystem();
}

void ACattleFlowGuide::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnregisterFromSubsystem();
    Super::EndPlay(EndPlayReason);
}

void ACattleFlowGuide::OnConstruction(const FTransform &Transform)
{
    Super::OnConstruction(Transform);
}

#if WITH_EDITOR
void ACattleFlowGuide::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();

        if (PropertyName == GET_MEMBER_NAME_CHECKED(ACattleFlowGuide, FlowmapResolution) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(ACattleFlowGuide, bUsePaintedFlowmap))
        {
            if (bUsePaintedFlowmap)
            {
                InitializeFlowmap();
            }
        }
    }
}
#endif

FVector ACattleFlowGuide::SampleFlowAtLocation(const FVector &Location, float &OutWeight) const
{
    OutWeight = 0.0f;

    if (!IsLocationInFlowArea(Location))
    {
        return FVector::ZeroVector;
    }

    // Calculate weight based on distance from center
    const FVector LocalLocation = InfluenceBox->GetComponentTransform().InverseTransformPosition(Location);
    const FVector Extent = InfluenceBox->GetUnscaledBoxExtent();

    // Weight falls off at edges
    const float XRatio = 1.0f - FMath::Abs(LocalLocation.X) / Extent.X;
    const float YRatio = 1.0f - FMath::Abs(LocalLocation.Y) / Extent.Y;
    OutWeight = FMath::Clamp(FMath::Min(XRatio, YRatio) * FlowStrength, 0.0f, 1.0f);

    if (bUsePaintedFlowmap && FlowmapData.Num() > 0)
    {
        // Sample from flowmap
        const FVector2D UV = WorldToFlowmapUV(Location);
        const FVector2D Flow2D = SampleFlowmapAtUV(UV);
        return FVector(Flow2D.X, Flow2D.Y, 0.0f);
    }
    else
    {
        // Use spline direction
        return GetSplineFlowDirection(Location);
    }
}

bool ACattleFlowGuide::IsLocationInFlowArea(const FVector &Location) const
{
    if (!InfluenceBox)
    {
        return false;
    }

    const FVector LocalLocation = InfluenceBox->GetComponentTransform().InverseTransformPosition(Location);
    const FVector Extent = InfluenceBox->GetUnscaledBoxExtent();

    return FMath::Abs(LocalLocation.X) <= Extent.X &&
           FMath::Abs(LocalLocation.Y) <= Extent.Y &&
           FMath::Abs(LocalLocation.Z) <= Extent.Z;
}

FVector ACattleFlowGuide::GetSplineFlowDirection(const FVector &Location) const
{
    if (!FlowSpline || FlowSpline->GetNumberOfSplinePoints() < 2)
    {
        return GetActorForwardVector();
    }

    // Find closest point on spline
    const float ClosestInputKey = FlowSpline->FindInputKeyClosestToWorldLocation(Location);

    // Get tangent at that point (direction of flow)
    const FVector Tangent = FlowSpline->GetTangentAtSplineInputKey(ClosestInputKey, ESplineCoordinateSpace::World);

    FVector Direction = Tangent.GetSafeNormal();
    Direction.Z = 0.0f; // Keep horizontal

    return Direction.GetSafeNormal();
}

void ACattleFlowGuide::InitializeFlowmap()
{
    // Create render target
    FlowmapRenderTarget = NewObject<UTextureRenderTarget2D>(this);
    FlowmapRenderTarget->InitCustomFormat(FlowmapResolution, FlowmapResolution, PF_FloatRGBA, false);
    FlowmapRenderTarget->UpdateResourceImmediate(true);

    // Initialize CPU-side data
    FlowmapData.SetNum(FlowmapResolution * FlowmapResolution);

    // Bake spline direction as initial flowmap
    BakeSplineToFlowmap();
}

void ACattleFlowGuide::PaintFlowAtLocation(const FVector &WorldLocation, const FVector &FlowDirection, float BrushRadius, float BrushStrength)
{
    if (FlowmapData.Num() == 0)
    {
        InitializeFlowmap();
    }

    const FVector2D CenterUV = WorldToFlowmapUV(WorldLocation);
    const FVector2D NormalizedFlow(FlowDirection.X, FlowDirection.Y);

    // Calculate brush radius in UV space
    const FVector Extent = InfluenceBox->GetUnscaledBoxExtent();
    const float BrushRadiusU = BrushRadius / (Extent.X * 2.0f);
    const float BrushRadiusV = BrushRadius / (Extent.Y * 2.0f);

    // Paint to all pixels within brush radius
    for (int32 Y = 0; Y < FlowmapResolution; ++Y)
    {
        for (int32 X = 0; X < FlowmapResolution; ++X)
        {
            const FVector2D PixelUV(
                static_cast<float>(X) / (FlowmapResolution - 1),
                static_cast<float>(Y) / (FlowmapResolution - 1));

            // Calculate distance from brush center
            const FVector2D Delta = PixelUV - CenterUV;
            const float NormalizedDist = FMath::Sqrt(
                FMath::Square(Delta.X / BrushRadiusU) +
                FMath::Square(Delta.Y / BrushRadiusV));

            if (NormalizedDist <= 1.0f)
            {
                // Smooth falloff
                const float Falloff = FMath::SmoothStep(0.0f, 1.0f, 1.0f - NormalizedDist);
                const float BlendFactor = Falloff * BrushStrength;

                const int32 Index = Y * FlowmapResolution + X;
                FlowmapData[Index] = FMath::Lerp(FlowmapData[Index], NormalizedFlow.GetSafeNormal(), BlendFactor);
            }
        }
    }
}

void ACattleFlowGuide::ClearFlowmap()
{
    BakeSplineToFlowmap();
}

void ACattleFlowGuide::BakeSplineToFlowmap()
{
    if (FlowmapData.Num() != FlowmapResolution * FlowmapResolution)
    {
        FlowmapData.SetNum(FlowmapResolution * FlowmapResolution);
    }

    for (int32 Y = 0; Y < FlowmapResolution; ++Y)
    {
        for (int32 X = 0; X < FlowmapResolution; ++X)
        {
            const FVector2D UV(
                static_cast<float>(X) / (FlowmapResolution - 1),
                static_cast<float>(Y) / (FlowmapResolution - 1));

            const FVector WorldLoc = FlowmapUVToWorld(UV);
            const FVector FlowDir = GetSplineFlowDirection(WorldLoc);

            const int32 Index = Y * FlowmapResolution + X;
            FlowmapData[Index] = FVector2D(FlowDir.X, FlowDir.Y);
        }
    }
}

void ACattleFlowGuide::DrawDebugFlow(float Duration) const
{
    UWorld *World = GetWorld();
    if (!World)
        return;

    // Draw influence box
    if (InfluenceBox)
    {
        DrawDebugBox(World, InfluenceBox->GetComponentLocation(), InfluenceBox->GetScaledBoxExtent(),
                     InfluenceBox->GetComponentQuat(), DebugColor, false, Duration, 0, 3.0f);
    }

    // Draw flow arrows
    if (FlowSpline && FlowSpline->GetNumberOfSplinePoints() >= 2)
    {
        const float SplineLength = FlowSpline->GetSplineLength();
        const int32 NumArrows = FMath::Max(5, static_cast<int32>(SplineLength / 200.0f));

        for (int32 i = 0; i < NumArrows; ++i)
        {
            const float Distance = (static_cast<float>(i) / (NumArrows - 1)) * SplineLength;
            const FVector Location = FlowSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
            const FVector Direction = FlowSpline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);

            DrawDebugDirectionalArrow(World, Location, Location + Direction * 100.0f, 30.0f, DebugColor, false, Duration, 0, 3.0f);
        }
    }
}

FVector2D ACattleFlowGuide::WorldToFlowmapUV(const FVector &WorldLocation) const
{
    if (!InfluenceBox)
    {
        return FVector2D(0.5f, 0.5f);
    }

    const FVector LocalLocation = InfluenceBox->GetComponentTransform().InverseTransformPosition(WorldLocation);
    const FVector Extent = InfluenceBox->GetUnscaledBoxExtent();

    // Map from [-Extent, Extent] to [0, 1]
    const float U = (LocalLocation.X + Extent.X) / (Extent.X * 2.0f);
    const float V = (LocalLocation.Y + Extent.Y) / (Extent.Y * 2.0f);

    return FVector2D(FMath::Clamp(U, 0.0f, 1.0f), FMath::Clamp(V, 0.0f, 1.0f));
}

FVector ACattleFlowGuide::FlowmapUVToWorld(const FVector2D &UV) const
{
    if (!InfluenceBox)
    {
        return GetActorLocation();
    }

    const FVector Extent = InfluenceBox->GetUnscaledBoxExtent();

    // Map from [0, 1] to [-Extent, Extent]
    const FVector LocalLocation(
        (UV.X * 2.0f - 1.0f) * Extent.X,
        (UV.Y * 2.0f - 1.0f) * Extent.Y,
        0.0f);

    return InfluenceBox->GetComponentTransform().TransformPosition(LocalLocation);
}

FVector2D ACattleFlowGuide::SampleFlowmapAtUV(const FVector2D &UV) const
{
    if (FlowmapData.Num() == 0)
    {
        return FVector2D::ZeroVector;
    }

    // Bilinear interpolation
    const float X = UV.X * (FlowmapResolution - 1);
    const float Y = UV.Y * (FlowmapResolution - 1);

    const int32 X0 = FMath::Clamp(static_cast<int32>(X), 0, FlowmapResolution - 1);
    const int32 X1 = FMath::Clamp(X0 + 1, 0, FlowmapResolution - 1);
    const int32 Y0 = FMath::Clamp(static_cast<int32>(Y), 0, FlowmapResolution - 1);
    const int32 Y1 = FMath::Clamp(Y0 + 1, 0, FlowmapResolution - 1);

    const float XFrac = X - X0;
    const float YFrac = Y - Y0;

    const FVector2D V00 = FlowmapData[Y0 * FlowmapResolution + X0];
    const FVector2D V10 = FlowmapData[Y0 * FlowmapResolution + X1];
    const FVector2D V01 = FlowmapData[Y1 * FlowmapResolution + X0];
    const FVector2D V11 = FlowmapData[Y1 * FlowmapResolution + X1];

    const FVector2D V0 = FMath::Lerp(V00, V10, XFrac);
    const FVector2D V1 = FMath::Lerp(V01, V11, XFrac);

    return FMath::Lerp(V0, V1, YFrac).GetSafeNormal();
}

UCattleAreaSubsystem *ACattleFlowGuide::GetAreaSubsystem() const
{
    if (UWorld *World = GetWorld())
    {
        return World->GetSubsystem<UCattleAreaSubsystem>();
    }
    return nullptr;
}

void ACattleFlowGuide::RegisterWithSubsystem()
{
    if (UCattleAreaSubsystem *Subsystem = GetAreaSubsystem())
    {
        Subsystem->RegisterFlowGuide(this);
    }
}

void ACattleFlowGuide::UnregisterFromSubsystem()
{
    if (UCattleAreaSubsystem *Subsystem = GetAreaSubsystem())
    {
        Subsystem->UnregisterFlowGuide(this);
    }
}
