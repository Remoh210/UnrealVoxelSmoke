
#include "Rendering/CustomSceneViewExtension.h"

#include "PixelShaderUtils.h"
#include "RenderGraphEvent.h"
#include "SceneRenderTargetParameters.h"
#include "SceneTexturesConfig.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "ShaderPasses/ColourExtractRenderPass.h"

DECLARE_GPU_DRAWCALL_STAT(ColourExtract); // Unreal Insights

FCustomSceneViewExtension::FCustomSceneViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister)
{
}

FCustomSceneViewExtension::~FCustomSceneViewExtension()
{
}

void FCustomSceneViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& View, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	// if(Pass == EPostProcessingPass::MotionBlur)
	// {
	// 	InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FCustomSceneViewExtension::CustomPostProcessFunction));
	// }
}

FScreenPassTexture FCustomSceneViewExtension::CustomPostProcessFunction(FRDGBuilder& GraphBuilder, const FSceneView& SceneView, const FPostProcessMaterialInputs& Inputs)
{
	// SceneViewExtension gives SceneView, not ViewInfo so we need to setup some basics
	const FSceneViewFamily& ViewFamily = *SceneView.Family;

	const FScreenPassTexture& SceneColor = FScreenPassTexture::CopyFromSlice(GraphBuilder, Inputs.GetInput(EPostProcessMaterialInput::SceneColor));

	// if (!SceneColor.IsValid() || CVarShaderOn.GetValueOnRenderThread() == 0)
	// {
	// 	return SceneColor;
	// }
	
	// Here starts the RDG stuff
	RDG_EVENT_SCOPE(GraphBuilder, "Custom Postprocess Effect");
	{
		FColourExtractPS::FParameters* Parameters = GraphBuilder.AllocParameters<FColourExtractPS::FParameters>();
		Parameters->SceneColorTexture = SceneColor.Texture;
		Parameters->SceneTextures = GetSceneTextureShaderParameters(SceneView);
	
		// Convert the target colour to Lab colour space here instead of in the shader
		// That will reduce the amount of calculations needed in the shader
		Parameters->TargetColour = FVector3f(1.0f, 0.0f, 0.0f);
		Parameters->View = SceneView.ViewUniformBuffer;
		// This will load the scene colour texture, and render to it
		Parameters->RenderTargets[0] = FRenderTargetBinding(SceneColor.Texture, ERenderTargetLoadAction::ELoad);

		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(ViewFamily.GetFeatureLevel());
		TShaderMapRef<FColourExtractPS> PixelShader(GlobalShaderMap);

		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder,
			GlobalShaderMap,
			RDG_EVENT_NAME("SimpleTextureCopy"),
			PixelShader,
			Parameters,
			SceneView.UnscaledViewRect
			);

		// Copy the output texture back to SceneColor
		// Returning the new texture as ScreenPassTexture doesn't work, so this is pretty fast alternative
		// Also with f.ex 'PrePostProcessPass_RenderThread' you get only input and something similar needs to be implemented then
		//AddCopyTexturePass(GraphBuilder, OutputTexture, SceneColor.Texture);
	}

	// The call expects ScreenPassTexture as a return, we return with the same texture as we started with, see AddCopyTexturePass above 
	return SceneColor;
}