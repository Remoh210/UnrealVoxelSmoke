// Copyright Epic Games, Inc. All Rights Reserved.

#include "VolumetricSmoke.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FVolumetricSmokeModule"

void FVolumetricSmokeModule::StartupModule()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("VolumetricSmoke"));
	if (!Plugin.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("VolumetricSmoke plugin not found!"));
		return;
	}

	const FString PluginShaderDir = FPaths::Combine(
	Plugin->GetBaseDir(),
	TEXT("Source/VolumetricSmoke/Private/Shaders")
	);

	if (!AllShaderSourceDirectoryMappings().Contains(TEXT("/CustomShaders")))
	{
		AddShaderSourceDirectoryMapping(TEXT("/CustomShaders"), PluginShaderDir);
		UE_LOG(LogTemp, Log, TEXT("Added shader directory mapping: %s"), *PluginShaderDir);
	}
}

void FVolumetricSmokeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVolumetricSmokeModule, VolumetricSmoke)