// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/VoxelizeSpaceComponent.h"

#include "TextureResource.h"
#include "Engine/TextureRenderTargetVolume.h"
#include "Subsystems/CustomShaderSubsystem.h"

// Sets default values for this component's properties
UVoxelizeSpaceComponent::UVoxelizeSpaceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UVoxelizeSpaceComponent::BeginPlay()
{
	Super::BeginPlay();

	UCustomShaderSubsystem* CustomShaderSubsystem = GEngine->GetEngineSubsystem<UCustomShaderSubsystem>();
	if (CustomShaderSubsystem)
	{
		ENQUEUE_RENDER_COMMAND(VoxelizeCmd)(
	[&](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);
			CustomShaderSubsystem->AddVoxelizationPass(GraphBuilder, VolumeEntry);
		}
		);
	}

	
}

void UVoxelizeSpaceComponent::DebugVoxelGridFromCS()
{

}

// Called every frame
void UVoxelizeSpaceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

