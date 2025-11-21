

#include "Subsystems/CustomShaderSubsystem.h"

#include "Engine/TextureRenderTargetVolume.h"
#include "ShaderPasses/VoxelizeSpaceComputePass.h"
#include "Rendering/CustomSceneViewExtension.h"

void UCustomShaderSubsystem::AddVoxelizationPass(FVoxelVolumeEntry& Entry)
{

	if (IsValid(Entry.VoxelTexture) == false)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("IsValid(Entry.VoxelTexture) == false")));
		
		return;
	}
	
	ENQUEUE_RENDER_COMMAND(VoxelizeCmd)(
[Entry](FRHICommandListImmediate& RHICmdList)
	{
		FRDGBuilder GraphBuilder(RHICmdList);
		FVoxelizeSmokeCS::FParameters* Params =
		GraphBuilder.AllocParameters<FVoxelizeSmokeCS::FParameters>();
	
	FRHITexture* RHITexture = Entry.VoxelTexture->GetRenderTargetResource()->GetTextureRHI();
	FRDGTextureRef RDGTexture = GraphBuilder.RegisterExternalTexture(
		CreateRenderTarget(RHITexture, TEXT("VoxelTex"))
	);

		FRDGTextureUAVRef VoxelUAV = GraphBuilder.CreateUAV(RDGTexture);
		Params->VoxelGridOut = VoxelUAV;
		Params->BoundsMin = Entry.BoundsMin;
		Params->BoundsMax = Entry.BoundsMax;
		Params->WorldToLocal = FMatrix44f(Entry.WorldToLocal);

		TShaderMapRef<FVoxelizeSmokeCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("VoxelizeVolume"),
		Shader,
		Params,
		FComputeShaderUtils::GetGroupCount(Entry.Resolution, FIntVector(8))
		);
	
		GraphBuilder.Execute();
	});
}

void UCustomShaderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FCoreDelegates::OnPostEngineInit.AddLambda([this]()
	{
		if (!CustomSceneViewExtension.IsValid())
		{
			CustomSceneViewExtension = FSceneViewExtensions::NewExtension<FCustomSceneViewExtension>();
		}
	});
}

void UCustomShaderSubsystem::Deinitialize()
{
	Super::Deinitialize();

	CustomSceneViewExtension.Reset();
	CustomSceneViewExtension = nullptr;
}
