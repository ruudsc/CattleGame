#include "LassoLoopActor.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "CattleGame/CattleGame.h"

ALassoLoopActor::ALassoLoopActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(RootComponent);
}

void ALassoLoopActor::BeginPlay()
{
	Super::BeginPlay();
}

void ALassoLoopActor::InitFromSpline(USplineComponent* SourceSpline)
{
	if (!SourceSpline)
	{
		return;
	}

	// 1. Copy spline points from source
	SplineComponent->ClearSplinePoints();
	int32 NumPoints = SourceSpline->GetNumberOfSplinePoints();
	
	for (int32 i = 0; i < NumPoints; i++)
	{
		FVector PointLoc = SourceSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		
		// Convert world location to local for this actor (assuming actor is at World Origin or attached properly)
		// Simpler: Just set world location on our points, assuming our actor transform is Identity or relative.
		// Actually, standard practice: Set actor transform to Source transform, then copy local points.
		// But SourceSpline might be component attached to moving actor.
		
		// Strategy:
		// 1. Set this Actor's transform to match the Target Actor (owner of SourceSpline).
		// 2. Or, keep this Actor at world origin? No, it should attach to Target.
		// Use local points if attached.
		
		SplineComponent->AddSplinePoint(
			SourceSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local),
			ESplineCoordinateSpace::Local,
			false
		);
		
		SplineComponent->SetSplinePointType(i, SourceSpline->GetSplinePointType(i), false);
	}

	SplineComponent->SetClosedLoop(SourceSpline->IsClosedLoop());
	SplineComponent->UpdateSpline();

	// 2. Clear old meshes
	for (USplineMeshComponent* Mesh : SplineMeshes)
	{
		if (Mesh)
		{
			Mesh->DestroyComponent();
		}
	}
	SplineMeshes.Empty();

	if (!RopeMeshAsset)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("LassoLoopActor: No RopeMeshAsset assigned"));
		return;
	}

	// 3. Generate Spline Meshes
	// Loop through points (or segments)
	int32 NumSegments = SplineComponent->GetNumberOfSplinePoints() - (SplineComponent->IsClosedLoop() ? 0 : 1);

	for (int32 i = 0; i < NumSegments; i++)
	{
		USplineMeshComponent* NewMesh = NewObject<USplineMeshComponent>(this);
		NewMesh->RegisterComponent();
		NewMesh->SetMobility(EComponentMobility::Movable);
		NewMesh->AttachToComponent(SplineComponent, FAttachmentTransformRules::KeepRelativeTransform);
		NewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // CRITICAL: Prevent physics explosion
		NewMesh->SetStaticMesh(RopeMeshAsset);
		
		if (RopeMaterial)
		{
			NewMesh->SetMaterial(0, RopeMaterial);
		}

		// Get points and tangents
		FVector StartPos, StartTan, EndPos, EndTan;
		SplineComponent->GetLocationAndTangentAtSplinePoint(i, StartPos, StartTan, ESplineCoordinateSpace::Local);
		
		int32 NextIndex = (i + 1) % SplineComponent->GetNumberOfSplinePoints();
		SplineComponent->GetLocationAndTangentAtSplinePoint(NextIndex, EndPos, EndTan, ESplineCoordinateSpace::Local);

		NewMesh->SetStartAndEnd(StartPos, StartTan, EndPos, EndTan, true);
		
		// Set scale for thickness
		// Assuming mesh is cylinder along X or Z. Standard is X forward.
		// If width is 1 unit in mesh, scale is RopeWidth.
		NewMesh->SetStartScale(FVector2D(RopeWidth, RopeWidth));
		NewMesh->SetEndScale(FVector2D(RopeWidth, RopeWidth));

		SplineMeshes.Add(NewMesh);
	}
}
