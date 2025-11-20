#pragma once

#include "CoreMinimal.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "GlobalShader.h"
#include "SceneTexturesConfig.h"
#include "ShaderParameterStruct.h"


// Shader parameters passed to HLSL
BEGIN_SHADER_PARAMETER_STRUCT(FVoxelizeParams, )
	SHADER_PARAMETER(FVector3f, BoundsMin)
	SHADER_PARAMETER(FVector3f, BoundsMax)
	SHADER_PARAMETER(FMatrix44f, WorldToLocal)
	SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float>, VoxelGridOut)
END_SHADER_PARAMETER_STRUCT()

// Forward declaration
struct FShaderCompilerEnvironment;

class FVoxelizeSmokeCS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FVoxelizeSmokeCS, Global,);
	using FParameters = FVoxelizeParams;
	SHADER_USE_PARAMETER_STRUCT(FVoxelizeSmokeCS, FGlobalShader);

public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		// SM5 = full DirectX 11/12 feature level (unlimited UAVs, RWTextures, etc.)
		// UAVs (RWTexture3D) require SM5 or higher.
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_X"), 8);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_Y"), 8);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_Z"), 8);
	}
};
