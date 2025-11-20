// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CanvasTypes.h"
#include "Components/SceneComponent.h"
#include "TextureRenderTargetVolumeResource.h"
#include "VoxelizeSpaceComponent.generated.h"



struct FVoxelVolumeEntry
{
	UVoxelizeSpaceComponent* Component;
	FRDGTextureRef VoxelTexture;
	FIntVector Resolution;
	FVector3f BoundsMin;
	FVector3f BoundsMax;
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

		
};
