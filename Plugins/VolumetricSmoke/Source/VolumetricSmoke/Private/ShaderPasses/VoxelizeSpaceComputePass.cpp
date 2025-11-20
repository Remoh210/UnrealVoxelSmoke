// Fill out your copyright notice in the Description page of Project Settings.


#include "ShaderPasses/VoxelizeSpaceComputePass.h"

// The location is set as VirtualMappingSetInModuleInitialise/private/NameOfShader.usf
// MainCS is the entry point for the compute shader
IMPLEMENT_SHADER_TYPE(, FVoxelizeSmokeCS, TEXT("/CustomShaders/VoxelizeShader.usf"), TEXT("MainCS"), SF_Compute);
