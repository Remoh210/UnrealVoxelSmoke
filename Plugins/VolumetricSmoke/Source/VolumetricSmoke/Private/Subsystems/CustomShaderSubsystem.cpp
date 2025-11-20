

#include "Subsystems/CustomShaderSubsystem.h"

#include "ShaderPasses/VoxelizeSpaceComputePass.h"
#include "Rendering/CustomSceneViewExtension.h"

void UCustomShaderSubsystem::AddVoxelizationPass(FRDGBuilder& GraphBuilder, FVoxelVolumeEntry& Entry)
{
	FVoxelizeSmokeCS::FParameters* Params =
	GraphBuilder.AllocParameters<FVoxelizeSmokeCS::FParameters>();

	FRDGTextureUAVRef VoxelUAV = GraphBuilder.CreateUAV(Entry.VoxelTexture);
	Params->VoxelGridOut = VoxelUAV;
	Params->BoundsMin = Entry.BoundsMin;
	Params->BoundsMax = Entry.BoundsMax;
	Params->WorldToLocal = FMatrix44f(Entry.Component->GetComponentToWorld().ToInverseMatrixWithScale());

	TShaderMapRef<FVoxelizeSmokeCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("VoxelizeVolume"),
		Shader,
		Params,
		FComputeShaderUtils::GetGroupCount(Entry.Resolution, FIntVector(8))
	);
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
