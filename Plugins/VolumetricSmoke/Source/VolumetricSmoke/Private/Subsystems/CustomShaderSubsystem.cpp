

#include "Subsystems/CustomShaderSubsystem.h"

#include "Rendering/CustomSceneViewExtension.h"

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
