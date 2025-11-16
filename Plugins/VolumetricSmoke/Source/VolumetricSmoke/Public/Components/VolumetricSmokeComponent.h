// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "PrimitiveSceneProxy.h"
#include "PrimitiveViewRelevance.h"
#include "VolumetricSmokeComponent.generated.h"

// Forward declarations
class FVolumetricSmokeSceneProxy;

/**
 * Simple voxel data structure
 */
USTRUCT(BlueprintType)
struct FSmokeVoxel
{
	GENERATED_BODY()

	// Density value (0.0 = empty, 1.0 = fully dense)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Density = 0.0f;

	FSmokeVoxel()
		: Density(0.0f)
	{
	}

	FSmokeVoxel(float InDensity)
		: Density(InDensity)
	{
	}
};

/**
 * Component that generates and manages a sphere-shaped voxel grid for volumetric smoke
 * Place this on an empty actor to create a smoke volume
 * Inherits from UPrimitiveComponent to support rendering
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VOLUMETRICSMOKE_API UVolumetricSmokeComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UVolumetricSmokeComponent(const FObjectInitializer& ObjectInitializer);

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	// UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

	/** Radius of the sphere in world units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Settings", meta = (ClampMin = "10.0", ClampMax = "1000.0"))
	float SphereRadius = 100.0f;

	/** Resolution of the voxel grid (voxels per axis) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Settings", meta = (ClampMin = "8", ClampMax = "128"))
	int32 VoxelResolution = 32;

	/** Whether to show debug visualization of voxels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugVisualization = true;

	/** Color for debug visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	FColor DebugColor = FColor::Red;

	/** Regenerate the voxel grid */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void RegenerateVoxels();

	/** Generate voxels at a specific world location (for smoke grenade explosion) */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void GenerateVoxelsAtLocation(const FVector& WorldLocation, float InRadius, int32 InResolution);

	/** Get voxel at grid coordinates */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	FSmokeVoxel GetVoxel(int32 X, int32 Y, int32 Z) const;

	/** Get voxel count */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	int32 GetVoxelCount() const { return VoxelGrid.Num(); }

protected:
	/** Generate voxels in a sphere shape */
	void GenerateSphereVoxels();

	/** Generate randomized colors for each voxel */
	void GenerateVoxelColors();

	/** Convert world position to voxel grid coordinates */
	FIntVector WorldToVoxel(const FVector& WorldPos) const;

	/** Convert voxel grid coordinates to world position */
	FVector VoxelToWorld(const FIntVector& VoxelCoord) const;

	/** Check if voxel coordinates are valid */
	bool IsValidVoxelCoord(const FIntVector& Coord) const;

	/** Draw debug visualization */
	void DrawDebugVisualization() const;

	/** Get local bounds of the voxel grid */
	FBoxSphereBounds GetLocalBounds() const;

private:
	// 3D voxel grid stored as 1D array: index = X + Y * Resolution + Z * Resolution * Resolution
	TArray<FSmokeVoxel> VoxelGrid;

	// Cached values
	int32 CurrentResolution = 0;
	float CurrentSphereRadius = 0.0f;
	
	// Cached voxel colors (randomized per voxel)
	TArray<FColor> VoxelColors;
	
	// Version number to track when voxels change (increments when voxels are regenerated)
	uint32 VoxelDataVersion = 0;
	
	// Friend class for scene proxy access
	friend class FVolumetricSmokeSceneProxy;
};

/**
 * Scene proxy for rendering volumetric smoke voxels as filled cubes
 */
class VOLUMETRICSMOKE_API FVolumetricSmokeSceneProxy : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	FVolumetricSmokeSceneProxy(const UVolumetricSmokeComponent* InComponent);
	
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	virtual uint32 GetMemoryFootprint(void) const override { return sizeof(*this) + GetAllocatedSize(); }
	uint32 GetAllocatedSize(void) const { return (uint32)FPrimitiveSceneProxy::GetAllocatedSize(); }

private:
	// Voxel data (cached from component when scene proxy is created)
	TArray<FVector> VoxelPositions;
	TArray<FColor> VoxelColors;
	float VoxelSize;
	int32 VoxelResolution;
	float SphereRadius;
	
	// Version tracking - scene proxy is recreated when this changes
	uint32 CachedVoxelDataVersion;
};

