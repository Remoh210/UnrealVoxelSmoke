#include "Components/VolumetricSmokeComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "PrimitiveSceneProxy.h"
#include "PrimitiveViewRelevance.h"
#include "SceneManagement.h"
#include "Materials/Material.h"
#include "DynamicMeshBuilder.h"
#include "Math/UnrealMathUtility.h"
#include "MeshMaterialShader.h"
#include "MeshPassProcessor.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h" 

UVolumetricSmokeComponent::UVolumetricSmokeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, SphereRadius(100.0f)
	, VoxelResolution(32)
	, bShowDebugVisualization(true)
	, DebugColor(FColor::Red)
	, CurrentResolution(0)
	, CurrentSphereRadius(0.0f)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	
	// Enable collision and rendering
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bUseEditorCompositing = true;
}

void UVolumetricSmokeComponent::OnRegister()
{
	Super::OnRegister();
	
	// Generate voxels when component is registered (works in editor and game)
	RegenerateVoxels();
}

void UVolumetricSmokeComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Regenerate voxels in case they weren't generated in editor
	if (VoxelGrid.Num() == 0)
	{
		RegenerateVoxels();
	}
}

void UVolumetricSmokeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	// Regenerate voxels when properties change in editor
	if (PropertyChangedEvent.Property)
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UVolumetricSmokeComponent, SphereRadius) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UVolumetricSmokeComponent, VoxelResolution))
		{
			RegenerateVoxels();
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UVolumetricSmokeComponent, SmokeMaterial))
		{
			// Material changed - update render state
			MarkRenderStateDirty();
		}
	}
}

void UVolumetricSmokeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Regenerate if parameters changed
	if (CurrentResolution != VoxelResolution || !FMath::IsNearlyEqual(CurrentSphereRadius, SphereRadius))
	{
		RegenerateVoxels();
	}

	// Draw debug visualization
	if (bShowDebugVisualization)
	{
		//DrawDebugVisualization();
	}

	UpdateVoxelsVisibility(DeltaTime);
}

void UVolumetricSmokeComponent::UpdateVoxelsVisibility(float DeltaTime)
{
	for (FSmokeVoxel& Voxel : SmokeVoxelArray)
	{
		const float OldVisibility = Voxel.Visibility;
		Voxel.Visibility = FMath::FInterpTo(OldVisibility, 1.0, DeltaTime, SmokeSpawnSpeed * Voxel.Density);
	}
}


void UVolumetricSmokeComponent::RegenerateVoxels()
{
	CurrentResolution = VoxelResolution;
	CurrentSphereRadius = SphereRadius;

	// Resize voxel grid
	const int32 TotalVoxels = VoxelResolution * VoxelResolution * VoxelResolution;
	VoxelGrid.SetNum(TotalVoxels);
	VoxelColors.SetNum(TotalVoxels);

	// Clear the smoke voxel array before regenerating
	SmokeVoxelArray.Empty();

	// Initialize all voxels to empty
	for (FSmokeVoxel& Voxel : VoxelGrid)
	{
		Voxel.Density = 0.0f;
	}

	// Generate sphere shape
	GenerateSphereVoxels();

	// Generate randomized colors for each voxel
	GenerateVoxelColors();

	// Increment version to indicate voxel data changed
	VoxelDataVersion++;

	// Update bounds and mark render state dirty (this recreates the scene proxy)
	UpdateBounds();
	MarkRenderStateDirty();

	UE_LOG(LogTemp, Log, TEXT("VolumetricSmoke: Generated %d voxels in sphere (Radius: %f, Resolution: %d)"), 
		GetVoxelCount(), SphereRadius, VoxelResolution);
}

void UVolumetricSmokeComponent::GenerateVoxelsAtLocation(const FVector& WorldLocation, float InRadius, int32 InResolution)
{
	// Set component location to the explosion point
	SetWorldLocation(WorldLocation);
	
	// Set voxel parameters
	SphereRadius = InRadius;
	VoxelResolution = InResolution;
	
	// Generate voxels once
	RegenerateVoxels();
	
	UE_LOG(LogTemp, Log, TEXT("VolumetricSmoke: Generated smoke at location %s"), *WorldLocation.ToString());
}

void UVolumetricSmokeComponent::GenerateSphereVoxels()
{
	const float VoxelSize = (SphereRadius * 2.0f) / VoxelResolution;
	const float SphereRadiusSquared = SphereRadius * SphereRadius;
	const FVector SphereCenter = FVector::ZeroVector; // Local space center
	const FVector Offset = FVector(SphereRadius); // Offset to center the grid


	if (bShowDebugVisualization)
	{
		DrawDebugSphere(GetWorld(),GetComponentTransform().TransformPosition(SphereCenter), SphereRadius, 12, FColor::Green, false, 5.0f , 0, 1.0f);
	}

	// Iterate through all voxel positions
	for (int32 Z = 0; Z < VoxelResolution; ++Z)
	{
		for (int32 Y = 0; Y < VoxelResolution; ++Y)
		{
			for (int32 X = 0; X < VoxelResolution; ++X)
			{
				// Convert voxel coordinates to local space position directly
				const FVector LocalPos = FVector(X, Y, Z) * VoxelSize - Offset;

				const FVector WorldPos = GetComponentTransform().TransformPosition(LocalPos);
				
				// Check if position is inside sphere
				const float DistanceSquared = FVector::DistSquared(LocalPos, SphereCenter);
				
				if (DistanceSquared <= SphereRadiusSquared)
				{



					if (IsVoxelNearStaticMesh(GetWorld(), WorldPos, VoxelSize))
					{
						if (bShowDebugVisualization)
						{
							DrawDebugBox(GetWorld(),WorldPos, FVector(VoxelSize * 0.5f), GetComponentQuat(), FColor::Red, false, 5.0f , 0, 5.0f);
						}
						continue;
					}
					
					// Calculate density based on distance from center (1.0 at center, 0.0 at edge)
					const float Distance = FMath::Sqrt(DistanceSquared);
					const float NormalizedDistance = Distance / SphereRadius;
					const float Density = 1.0f - FMath::Clamp(NormalizedDistance, 0.0f, 1.0f);
					
					// Store voxel
					const int32 Index = X + Y * VoxelResolution + Z * VoxelResolution * VoxelResolution;
					VoxelGrid[Index].Density = Density;
					// Start all voxels with visibility 0 so they all fade in gradually
					// This prevents edge voxels from appearing instantly
					VoxelGrid[Index].Visibility = 0.0f;
					VoxelGrid[Index].LocalPosition = LocalPos;

					// Store Smoke filled Voxels - explicitly ensure visibility is 0
					FSmokeVoxel SmokeVoxel = VoxelGrid[Index];
					SmokeVoxel.Visibility = 0.0f; // Explicitly set to 0 to prevent any instant appearance
					SmokeVoxelArray.Add(SmokeVoxel);
				}
			}
		}
	}
}

bool UVolumetricSmokeComponent::IsVoxelNearStaticMesh(UWorld* World, const FVector& VoxelPos, float Radius)
{
	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectTypes(ECollisionChannel::ECC_WorldStatic);
	FCollisionQueryParams Params;
	Params.bTraceComplex = true;
 
	bool bOverlap = World->OverlapMultiByChannel(
		OverlapResults, VoxelPos, FQuat::Identity,
		ECC_Visibility, FCollisionShape::MakeSphere(Radius), Params);
 
	return bOverlap;
}

void UVolumetricSmokeComponent::GenerateVoxelColors()
{
	// Generate randomized colors for each voxel
	for (int32 Index = 0; Index < SmokeVoxelArray.Num(); ++Index)
	{
		if (SmokeVoxelArray[Index].Density > 0.0f)
		{
			// Generate random color components
			const uint8 R = FMath::RandRange(50, 255);
			const uint8 G = FMath::RandRange(50, 255);
			const uint8 B = FMath::RandRange(50, 255);
			SmokeVoxelArray[Index].Colour = FColor(R, G, B, 255);
		}
		else
		{
			SmokeVoxelArray[Index].Colour = FColor::Black;
		}
	}
}

FSmokeVoxel UVolumetricSmokeComponent::GetVoxel(int32 X, int32 Y, int32 Z) const
{
	if (!IsValidVoxelCoord(FIntVector(X, Y, Z)))
	{
		return FSmokeVoxel(0.0f);
	}

	const int32 Index = X + Y * VoxelResolution + Z * VoxelResolution * VoxelResolution;
	return VoxelGrid[Index];
}

FIntVector UVolumetricSmokeComponent::WorldToVoxel(const FVector& WorldPos) const
{
	const FVector LocalPos = GetComponentTransform().InverseTransformPosition(WorldPos);
	const float VoxelSize = (SphereRadius * 2.0f) / VoxelResolution;
	const FVector Offset = FVector(SphereRadius); // Offset to center the grid
	
	const FVector GridPos = (LocalPos + Offset) / VoxelSize;
	return FIntVector(
		FMath::FloorToInt(GridPos.X),
		FMath::FloorToInt(GridPos.Y),
		FMath::FloorToInt(GridPos.Z)
	);
}

FVector UVolumetricSmokeComponent::VoxelToWorld(const FIntVector& VoxelCoord) const
{
	const float VoxelSize = (SphereRadius * 2.0f) / VoxelResolution;
	const FVector Offset = FVector(SphereRadius); // Offset to center the grid
	
	const FVector LocalPos = FVector(VoxelCoord) * VoxelSize - Offset;
	return GetComponentTransform().TransformPosition(LocalPos);
}

bool UVolumetricSmokeComponent::IsValidVoxelCoord(const FIntVector& Coord) const
{
	return Coord.X >= 0 && Coord.X < VoxelResolution &&
		   Coord.Y >= 0 && Coord.Y < VoxelResolution &&
		   Coord.Z >= 0 && Coord.Z < VoxelResolution;
}

void UVolumetricSmokeComponent::DrawDebugVisualization() const
{
	if (!GetWorld() || VoxelGrid.Num() == 0)
	{
		return;
	}
	
	const float VoxelSize = (SphereRadius * 2.0f) / VoxelResolution;
	for (const FSmokeVoxel& SmokeVoxel : SmokeVoxelArray)
	{
		if (SmokeVoxel.Visibility < 0.5)
		{
			continue;
		}

		const FVector LocalPos = SmokeVoxel.LocalPosition;
		const FVector WorldPos = GetComponentTransform().TransformPosition(LocalPos);
		;
		const FVector BoxExtent = FVector(VoxelSize * 0.5f);
					
		// Color intensity based on density
		const FColor VoxelColor = FColor(
			DebugColor.R,
			DebugColor.G,
			DebugColor.B,
			FMath::Clamp(FMath::RoundToInt(SmokeVoxel.Density * 255.0f), 0, 255)
		);

		DrawDebugBox(
			GetWorld(),
			WorldPos,
			BoxExtent,
			GetComponentQuat(),
			VoxelColor,
			false,
			0.0f,
			0,
			1.0f
		);
	}
}

FPrimitiveSceneProxy* UVolumetricSmokeComponent::CreateSceneProxy()
{
	return new FVolumetricSmokeSceneProxy(this);
}

FBoxSphereBounds UVolumetricSmokeComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FBoxSphereBounds LocalBounds = GetLocalBounds();
	return LocalBounds.TransformBy(LocalToWorld);
}

FBoxSphereBounds UVolumetricSmokeComponent::GetLocalBounds() const
{
	// Return bounds based on sphere radius
	const FVector BoxExtent = FVector(SphereRadius);
	const FBox BoundingBox(-BoxExtent, BoxExtent);
	return FBoxSphereBounds(BoundingBox);
}

void UVolumetricSmokeComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (SmokeMaterial)
	{
		OutMaterials.Add(SmokeMaterial);
	}
	Super::GetUsedMaterials(OutMaterials, bGetDebugMaterials);
}

// ============================================================================
// FVolumetricSmokeSceneProxy Implementation
// ============================================================================

FVolumetricSmokeSceneProxy::FVolumetricSmokeSceneProxy(UVolumetricSmokeComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
	, VoxelSize(0.0f)
	, VoxelResolution(0)
	, SphereRadius(0.0f)
	, CachedVoxelDataVersion(0)
{
	// Copy voxel data from component
	VoxelResolution = InComponent->VoxelResolution;
	SphereRadius = InComponent->SphereRadius;
	CachedVoxelDataVersion = InComponent->VoxelDataVersion;
	SmokeComp = InComponent;
	
	// Calculate voxel size (critical - without this, cubes have zero size!)
	if (VoxelResolution > 0)
	{
		VoxelSize = (SphereRadius * 2.0f) / VoxelResolution;
	}
	else
	{
		VoxelSize = 0.0f;
	}
	
	bWillEverBeLit = true;
}

void FVolumetricSmokeSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{

	// NOTE: This is called every frame, but it's efficient because:
	// 1. Voxel data (positions/colors) is cached in the scene proxy (copied once when proxy is created)
	// 2. We only rebuild the mesh geometry here, not the voxel data
	// 3. For smoke grenade: voxels are generated ONCE on explosion, mesh is built from cached data each frame
	if (IsValid(SmokeComp) == false)
	{
		return;
	}

	TArray<FSmokeVoxel>& SmokeVoxelArray = SmokeComp->SmokeVoxelArray;
	
	// Early return if no voxels to render
	if (SmokeVoxelArray.Num() == 0)
	{
		return;
	}
	
	
	// Use assigned smoke material, or fall back to default material
	UMaterialInterface* Material = SmokeComp->SmokeMaterial;
	if (!Material)
	{
		UE_LOG(LogTemp, Warning, TEXT("VolumetricSmoke: No material assigned, using default material"));
		Material = UMaterial::GetDefaultMaterial(MD_Surface);
	}
	if (!Material)
	{
		UE_LOG(LogTemp, Error, TEXT("VolumetricSmoke: Failed to get material"));
		return;
	}
	
	// Debug: Log material info
	UE_LOG(LogTemp, Log, TEXT("VolumetricSmoke: Rendering %d voxels with material %s"), 
		SmokeVoxelArray.Num(), *Material->GetName());
	
	const float HalfVoxelSize = VoxelSize * 0.5f;
	
	// Create a single mesh builder for all cubes (batched - one draw call!)
	// This builds mesh geometry from cached voxel data
	FDynamicMeshBuilder MeshBuilder(GetScene().GetFeatureLevel());
	const FMaterialRenderProxy* MaterialRenderProxy = Material->GetRenderProxy();
	
	// Generate cube geometry for each voxel - all added to the same mesh builder
	for (int32 VoxelIndex = 0; VoxelIndex < SmokeVoxelArray.Num(); ++VoxelIndex)
	{
		// Bounds check
		if (VoxelIndex >= SmokeVoxelArray.Num())
		{
			continue;
		}
		
		if (SmokeVoxelArray[VoxelIndex].Visibility < 0.5)
		{
			continue;
		}
		
		const FVector& VoxelPos = SmokeVoxelArray[VoxelIndex].LocalPosition;
		
		// Calculate vertex color based on density and visibility for proper smoke appearance
		// Use density to control opacity/intensity, visibility for fade-in effect
		const float Density = SmokeVoxelArray[VoxelIndex].Density;
		const float Visibility = SmokeVoxelArray[VoxelIndex].Visibility;
		const float FinalAlpha = FMath::Clamp(Density, 0.0f, 1.0f);
		
		// Use a smoke-like color (grayish white) with density-based variation
		// You can adjust these values or use the stored Colour if preferred
		const uint8 Intensity = 255 * Density;
		const FColor VoxelColor = FColor(Intensity, Intensity, Intensity, Intensity);
		
		// Define cube vertices (8 corners) - use FVector3f for mesh vertices
		const FVector3f CubeExtent(HalfVoxelSize);
		const FVector3f VoxelPosF = FVector3f(VoxelPos);
		const FVector3f Vertices[8] = {
			VoxelPosF + FVector3f(-CubeExtent.X, -CubeExtent.Y, -CubeExtent.Z), // 0
			VoxelPosF + FVector3f( CubeExtent.X, -CubeExtent.Y, -CubeExtent.Z), // 1
			VoxelPosF + FVector3f( CubeExtent.X,  CubeExtent.Y, -CubeExtent.Z), // 2
			VoxelPosF + FVector3f(-CubeExtent.X,  CubeExtent.Y, -CubeExtent.Z), // 3
			VoxelPosF + FVector3f(-CubeExtent.X, -CubeExtent.Y,  CubeExtent.Z), // 4
			VoxelPosF + FVector3f( CubeExtent.X, -CubeExtent.Y,  CubeExtent.Z), // 5
			VoxelPosF + FVector3f( CubeExtent.X,  CubeExtent.Y,  CubeExtent.Z), // 6
			VoxelPosF + FVector3f(-CubeExtent.X,  CubeExtent.Y,  CubeExtent.Z)  // 7
		};
		
		// Add cube faces with proper normals (each face needs its own vertices for correct normals)
		// We'll create vertices per face to have correct normals
		
		// Bottom face (-Z)
		{
			FVector3f Tangent = FVector3f(1, 0, 0);
			FVector3f Binormal = FVector3f(0, 1, 0);
			FDynamicMeshVertex V0(Vertices[0], Tangent, Binormal, FVector2f(0, 0), VoxelColor);
			FDynamicMeshVertex V1(Vertices[1], Tangent, Binormal, FVector2f(1, 0), VoxelColor);
			FDynamicMeshVertex V2(Vertices[2], Tangent, Binormal, FVector2f(1, 1), VoxelColor);
			FDynamicMeshVertex V3(Vertices[3], Tangent, Binormal, FVector2f(0, 1), VoxelColor);
			int32 BaseIdx = MeshBuilder.AddVertex(V0);
			MeshBuilder.AddVertex(V1);
			MeshBuilder.AddVertex(V2);
			MeshBuilder.AddVertex(V3);
			MeshBuilder.AddTriangle(BaseIdx, BaseIdx + 1, BaseIdx + 2);
			MeshBuilder.AddTriangle(BaseIdx, BaseIdx + 2, BaseIdx + 3);
		}
		
		// Top face (+Z)
		{
			FVector3f Tangent = FVector3f(1, 0, 0);
			FVector3f Binormal = FVector3f(0, 1, 0);
			FDynamicMeshVertex V0(Vertices[4], Tangent, Binormal, FVector2f(0, 0), VoxelColor);
			FDynamicMeshVertex V1(Vertices[5], Tangent, Binormal, FVector2f(1, 0), VoxelColor);
			FDynamicMeshVertex V2(Vertices[6], Tangent, Binormal, FVector2f(1, 1), VoxelColor);
			FDynamicMeshVertex V3(Vertices[7], Tangent, Binormal, FVector2f(0, 1), VoxelColor);
			int32 BaseIdx = MeshBuilder.AddVertex(V0);
			MeshBuilder.AddVertex(V1);
			MeshBuilder.AddVertex(V2);
			MeshBuilder.AddVertex(V3);
			MeshBuilder.AddTriangle(BaseIdx, BaseIdx + 2, BaseIdx + 1);
			MeshBuilder.AddTriangle(BaseIdx, BaseIdx + 3, BaseIdx + 2);
		}
		
		// Front face (-Y)
		{
			FVector3f Tangent = FVector3f(1, 0, 0);
			FVector3f Binormal = FVector3f(0, 0, 1);
			FDynamicMeshVertex V0(Vertices[0], Tangent, Binormal, FVector2f(0, 0), VoxelColor);
			FDynamicMeshVertex V1(Vertices[4], Tangent, Binormal, FVector2f(0, 1), VoxelColor);
			FDynamicMeshVertex V2(Vertices[5], Tangent, Binormal, FVector2f(1, 1), VoxelColor);
			FDynamicMeshVertex V3(Vertices[1], Tangent, Binormal, FVector2f(1, 0), VoxelColor);
			int32 BaseIdx = MeshBuilder.AddVertex(V0);
			MeshBuilder.AddVertex(V1);
			MeshBuilder.AddVertex(V2);
			MeshBuilder.AddVertex(V3);
			MeshBuilder.AddTriangle(BaseIdx, BaseIdx + 1, BaseIdx + 2);
			MeshBuilder.AddTriangle(BaseIdx, BaseIdx + 2, BaseIdx + 3);
		}
		
		// Back face (+Y)
		{
			FVector3f Tangent = FVector3f(1, 0, 0);
			FVector3f Binormal = FVector3f(0, 0, 1);
			FDynamicMeshVertex V0(Vertices[2], Tangent, Binormal, FVector2f(0, 0), VoxelColor);
			FDynamicMeshVertex V1(Vertices[6], Tangent, Binormal, FVector2f(1, 0), VoxelColor);
			FDynamicMeshVertex V2(Vertices[7], Tangent, Binormal, FVector2f(1, 1), VoxelColor);
			FDynamicMeshVertex V3(Vertices[3], Tangent, Binormal, FVector2f(0, 1), VoxelColor);
			int32 BaseIdx = MeshBuilder.AddVertex(V0);
			MeshBuilder.AddVertex(V1);
			MeshBuilder.AddVertex(V2);
			MeshBuilder.AddVertex(V3);
			MeshBuilder.AddTriangle(BaseIdx, BaseIdx + 1, BaseIdx + 2);
			MeshBuilder.AddTriangle(BaseIdx, BaseIdx + 2, BaseIdx + 3);
		}
		
		// Left face (-X)
		{
			FVector3f Tangent = FVector3f(0, 1, 0);
			FVector3f Binormal = FVector3f(0, 0, 1);
			FDynamicMeshVertex V0(Vertices[0], Tangent, Binormal, FVector2f(0, 0), VoxelColor);
			FDynamicMeshVertex V1(Vertices[3], Tangent, Binormal, FVector2f(1, 0), VoxelColor);
			FDynamicMeshVertex V2(Vertices[7], Tangent, Binormal, FVector2f(1, 1), VoxelColor);
			FDynamicMeshVertex V3(Vertices[4], Tangent, Binormal, FVector2f(0, 1), VoxelColor);
			int32 BaseIdx = MeshBuilder.AddVertex(V0);
			MeshBuilder.AddVertex(V1);
			MeshBuilder.AddVertex(V2);
			MeshBuilder.AddVertex(V3);
			MeshBuilder.AddTriangle(BaseIdx, BaseIdx + 1, BaseIdx + 2);
			MeshBuilder.AddTriangle(BaseIdx, BaseIdx + 2, BaseIdx + 3);
		}
		
		// Right face (+X)
		{
			FVector3f Tangent = FVector3f(0, 1, 0);
			FVector3f Binormal = FVector3f(0, 0, 1);
			FDynamicMeshVertex V0(Vertices[1], Tangent, Binormal, FVector2f(0, 0), VoxelColor);
			FDynamicMeshVertex V1(Vertices[5], Tangent, Binormal, FVector2f(1, 0), VoxelColor);
			FDynamicMeshVertex V2(Vertices[6], Tangent, Binormal, FVector2f(1, 1), VoxelColor);
			FDynamicMeshVertex V3(Vertices[2], Tangent, Binormal, FVector2f(0, 1), VoxelColor);
			int32 BaseIdx = MeshBuilder.AddVertex(V0);
			MeshBuilder.AddVertex(V1);
			MeshBuilder.AddVertex(V2);
			MeshBuilder.AddVertex(V3);
			MeshBuilder.AddTriangle(BaseIdx, BaseIdx + 1, BaseIdx + 2);
			MeshBuilder.AddTriangle(BaseIdx, BaseIdx + 2, BaseIdx + 3);
		}
	}
	
	// Build the single batched mesh for each view (much more efficient - one draw call instead of hundreds!)
	// Use SDPG_World for translucent rendering - bOpaque = false in view relevance handles translucency
	// IMPORTANT: Your material must have Vertex Color node connected to Base Color and Opacity inputs
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			MeshBuilder.GetMesh(
				GetLocalToWorld(), // Use component's transform
				MaterialRenderProxy,
				SDPG_World, // Depth priority group - translucency handled by bOpaque = false
				false,      // bDisableBackfaceCulling
				false,      // bReceivesDecals
				ViewIndex,
				Collector
			);
		}
	}
}

FPrimitiveViewRelevance FVolumetricSmokeSceneProxy::GetViewRelevance(const FSceneView* View) const
{

	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bDynamicRelevance = true;
	Result.bSeparateTranslucency = true;
	Result.bNormalTranslucency = true;
	Result.bRenderInMainPass = true;
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bTranslucentSelfShadow = true;
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	
	// Smoke is always translucent
	Result.bOpaque = false;
	Result.bMasked = false;
	return Result;
}
