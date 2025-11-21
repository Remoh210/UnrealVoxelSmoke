// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CanvasTypes.h"
#include "Components/SceneComponent.h"
#include "TextureRenderTargetVolumeResource.h"
#include "VoxelizeSpaceComponent.generated.h"



struct FVoxelVolumeEntry
{

	UTextureRenderTargetVolume* VoxelTexture = nullptr;
	FIntVector Resolution;
	FVector3f BoundsMin;
	FVector3f BoundsMax;
	FMatrix44f WorldToLocal;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VOLUMETRICSMOKE_API UVoxelizeSpaceComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVoxelizeSpaceComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void DebugVoxelGridFromCS();

	UPROPERTY()
	UTextureRenderTargetVolume* VoxelTarget;

public:
	FVoxelVolumeEntry VolumeEntry;
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;




#pragma region debug
	/** Mesh to apply debug material on */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelDebug")
	//UMeshComponent* TargetMesh;

	/** Base material with a Texture Parameter for voxel volume */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelDebug")
	UMaterialInterface* BaseMaterial;
	
	UPROPERTY()
	UMaterialInstanceDynamic* MID;

	/** Parameter name in the material for the volume texture */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelDebug")
	FName TextureParameterName = TEXT("VoxelVolumeTex");

	/** Apply a voxel texture to the material */
	UFUNCTION(BlueprintCallable, Category="VoxelDebug")
	void ApplyVoxelTexture(UTextureRenderTargetVolume* VoxelTexture);
#pragma endregion 

		
};
