// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/VoxelizeSpaceComponent.h"

#include "TextureResource.h"
#include "TimerManager.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTargetVolume.h"
#include "Materials/MaterialInstanceDynamic.h"
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

	UTextureRenderTargetVolume* Tex = NewObject<UTextureRenderTargetVolume>();
	
	VolumeEntry.Resolution = FIntVector(720, 720, 720);
	VolumeEntry.BoundsMin = FVector3f(-120, -120, -120);
	VolumeEntry.BoundsMax = FVector3f(120, 120, 120);

	// Needed to prevent GC if storing in struct
	Tex->AddToRoot();

	// Texture properties
	Tex->bHDR = true;                         // HDR gives you PF_FloatRGBA unless overridden
	Tex->bForceLinearGamma = true;            // No SRGB
	Tex->bSupportsUAV = true;                 // Needed for compute shader UAV

	// You can explicitly control format:
	Tex->OverrideFormat = PF_R32_FLOAT;       // single channel float, perfect for density

	// Init volume texture
	Tex->Init(VolumeEntry.Resolution.X, VolumeEntry.Resolution.Y, VolumeEntry.Resolution.Z, Tex->OverrideFormat);

	// Allocate RHI resource immediately
	Tex->UpdateResourceImmediate(true);

	VolumeEntry.VoxelTexture = Tex;
	
	CustomShaderSubsystem->AddVoxelizationPass(VolumeEntry);


	// Delay to next frame
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
	{
		ApplyVoxelTexture(VolumeEntry.VoxelTexture);
	}, 1.f, false);

	//Component->GetComponentToWorld().ToInverseMatrixWithScale()
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

void UVoxelizeSpaceComponent::ApplyVoxelTexture(UTextureRenderTargetVolume* VoxelTexture)
{
	// Inside your component
	UStaticMeshComponent* MeshComp = GetOwner()->FindComponentByClass<UStaticMeshComponent>();

	if (MeshComp)
	{
		UE_LOG(LogTemp, Log, TEXT("Found StaticMeshComponent: %s"), *MeshComp->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Owner has no StaticMeshComponent"));
	}
	
	if (!MeshComp || !BaseMaterial || !VoxelTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("VoxelDebugMaterialComponent: Missing input."));
		return;
	}

	// Create MID if not created yet
	if (!MID)
	{
		MID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		MeshComp->SetMaterial(0, MID);
	}

	// Set the volume texture parameter
	MID->SetTextureParameterValue(TextureParameterName, VoxelTexture);
}

