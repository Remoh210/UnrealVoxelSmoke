// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Rendering/CustomSceneViewExtension.h"
#include "CustomShaderSubsystem.generated.h"

// Forward declarations

/**
 * 
 */
UCLASS()
class VOLUMETRICSMOKE_API UCustomShaderSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	TSharedPtr<FCustomSceneViewExtension, ESPMode::ThreadSafe> CustomSceneViewExtension;
};
