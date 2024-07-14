#include "PivotPainterDestroyCompute.h"
#include "PivotPainterDestroyModule/Public/PivotPainterDestroyCompute/PivotPainterDestroyCompute.h"
#include "PixelShaderUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MeshDrawShaderBindings.h"
#include "RHIGPUReadback.h"
#include "MeshPassUtils.h"
#include "MaterialShader.h"

void FPivotPainterDestroyComputeInterface::CopyTextureToRenderTarget(UTexture2D* Input,
	FRenderTarget* Target,
	FRHICommandListImmediate& RHICmdList)
{
	FRDGBuilder GraphBuilder(RHICmdList);
	const FTextureRHIRef InputTexRHI =   Input->GetResource()->TextureRHI->GetTexture2D();
	const FTextureRHIRef TargetTexRHI =  Target->GetRenderTargetTexture();
	if(InputTexRHI->GetFormat() == TargetTexRHI->GetFormat())
	{
		const FRDGTextureRef InputTexture =RegisterExternalTexture(GraphBuilder,  Input->GetResource()->TextureRHI->GetTexture2D(), TEXT("InputTexture"));;
		const FRDGTextureRef TargetTexture = RegisterExternalTexture(GraphBuilder, Target->GetRenderTargetTexture(), TEXT("TargetTexture"));
		AddCopyTexturePass(GraphBuilder, InputTexture, TargetTexture, FRHICopyTextureInfo());
	}
	GraphBuilder.Execute();
}
DECLARE_STATS_GROUP(TEXT("PivotPainterDestroyCompute"), STATGROUP_PivotPainterDestroyCompute, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("PivotPainterDestroyCompute Execute"), STAT_PivotPainterDestroyCompute_Execute, STATGROUP_PivotPainterDestroyCompute);

// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
class PIVOTPAINTERDESTROYMODULE_API FPivotPainterDestroyResetBuffers: public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FPivotPainterDestroyResetBuffers);
	SHADER_USE_PARAMETER_STRUCT(FPivotPainterDestroyResetBuffers, FGlobalShader);
	
	
	class PivotPainterDestroyPivotPainterDestroyResetBuffers_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<
		PivotPainterDestroyPivotPainterDestroyResetBuffers_Perm_TEST
	>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, RWClosestHit)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, RWChunksToDisableCount)
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);
	}
private:
};
class PIVOTPAINTERDESTROYMODULE_API FPivotPainterDestroyFindClosestHit: public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FPivotPainterDestroyFindClosestHit);
	SHADER_USE_PARAMETER_STRUCT(FPivotPainterDestroyFindClosestHit, FGlobalShader);
	
	
	class FPivotPainterDestroyFindClosestHit_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<
		FPivotPainterDestroyFindClosestHit_Perm_TEST
	>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/*
		* Here's where you define one or more of the input parameters for your shader.
		* Some examples:
		*/
		// SHADER_PARAMETER(uint32, MyUint32) // On the shader side: uint32 MyUint32;
		// SHADER_PARAMETER(FVector3f, MyVector) // On the shader side: float3 MyVector;

		// SHADER_PARAMETER_TEXTURE(Texture2D, MyTexture) // On the shader side: Texture2D<float4> MyTexture; (float4 should be whatever you expect each pixel in the texture to be, in this case float4(R,G,B,A) for 4 channels)
		// SHADER_PARAMETER_SAMPLER(SamplerState, MyTextureSampler) // On the shader side: SamplerState MySampler; // CPP side: TStaticSamplerState<ESamplerFilter::SF_Bilinear>::GetRHI();

		// SHADER_PARAMETER_ARRAY(float, MyFloatArray, [3]) // On the shader side: float MyFloatArray[3];

		// SHADER_PARAMETER_UAV(RWTexture2D<FVector4f>, MyTextureUAV) // On the shader side: RWTexture2D<float4> MyTextureUAV;
		// SHADER_PARAMETER_UAV(RWStructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWStructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_UAV(RWBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWBuffer<FMyCustomStruct> MyCustomStructs;

		// SHADER_PARAMETER_SRV(StructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: StructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Buffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: Buffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Texture2D<FVector4f>, MyReadOnlyTexture) // On the shader side: Texture2D<float4> MyReadOnlyTexture;

		// SHADER_PARAMETER_STRUCT_REF(FMyCustomStruct, MyCustomStruct)

		
		SHADER_PARAMETER(float, Gametime)
		SHADER_PARAMETER(FVector3f, RayOrigin)
		SHADER_PARAMETER(FVector3f, RayDirection)
		SHADER_PARAMETER(FMatrix44f, ObjectMatrix)
		SHADER_PARAMETER_TEXTURE(Texture2D, PivotPainterOriginsExtent)
		SHADER_PARAMETER_TEXTURE(Texture2D, Timestamp)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, RWClosestHit)

	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		/*
		* Here you define constants that can be used statically in the shader code.
		* Example:
		*/
		// OutEnvironment.SetDefine(TEXT("MY_CUSTOM_CONST"), TEXT("1"));

		/*
		* These defines are used in the thread count section of our shader
		*/
		OutEnvironment.SetDefine(TEXT("THREADS_X_SHADER0"), NUM_THREADS_PivotPainterDestroyCompute_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y_SHADER0"), NUM_THREADS_PivotPainterDestroyCompute_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z_SHADER0"), NUM_THREADS_PivotPainterDestroyCompute_Z);

		// This shader must support typed UAV load and we are testing if it is supported at runtime using RHIIsTypedUAVLoadSupported
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);

		// FForwardLightingParameters::ModifyCompilationEnvironment(Parameters.Platform, OutEnvironment);
	}
private:
};
class PIVOTPAINTERDESTROYMODULE_API FPivotPainterDestroyAnimateChunks: public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FPivotPainterDestroyAnimateChunks);
	SHADER_USE_PARAMETER_STRUCT(FPivotPainterDestroyAnimateChunks, FGlobalShader);
	
	
	class FPivotPainterDestroyAnimateChunks_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<
		FPivotPainterDestroyAnimateChunks_Perm_TEST
	>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/*
		* Here's where you define one or more of the input parameters for your shader.
		* Some examples:
		*/
		// SHADER_PARAMETER(uint32, MyUint32) // On the shader side: uint32 MyUint32;
		// SHADER_PARAMETER(FVector3f, MyVector) // On the shader side: float3 MyVector;

		// SHADER_PARAMETER_TEXTURE(Texture2D, MyTexture) // On the shader side: Texture2D<float4> MyTexture; (float4 should be whatever you expect each pixel in the texture to be, in this case float4(R,G,B,A) for 4 channels)
		// SHADER_PARAMETER_SAMPLER(SamplerState, MyTextureSampler) // On the shader side: SamplerState MySampler; // CPP side: TStaticSamplerState<ESamplerFilter::SF_Bilinear>::GetRHI();

		// SHADER_PARAMETER_ARRAY(float, MyFloatArray, [3]) // On the shader side: float MyFloatArray[3];

		// SHADER_PARAMETER_UAV(RWTexture2D<FVector4f>, MyTextureUAV) // On the shader side: RWTexture2D<float4> MyTextureUAV;
		// SHADER_PARAMETER_UAV(RWStructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWStructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_UAV(RWBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWBuffer<FMyCustomStruct> MyCustomStructs;

		// SHADER_PARAMETER_SRV(StructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: StructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Buffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: Buffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Texture2D<FVector4f>, MyReadOnlyTexture) // On the shader side: Texture2D<float4> MyReadOnlyTexture;

		// SHADER_PARAMETER_STRUCT_REF(FMyCustomStruct, MyCustomStruct)

		
		SHADER_PARAMETER(float, Gametime)
		SHADER_PARAMETER(float, InverseSpreadDistance)
		SHADER_PARAMETER(float, MaxDelay)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture<float>, RWTimestamp)
		SHADER_PARAMETER(FVector3f, RayOrigin)
		SHADER_PARAMETER(FVector3f, RayDirection)
		SHADER_PARAMETER(FMatrix44f, ObjectMatrix)
		SHADER_PARAMETER_TEXTURE(Texture2D, PivotPainterOriginsExtent)
		SHADER_PARAMETER_TEXTURE(Texture2D, Timestamp)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, ClosestHit)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(Texture2D, RWNormalAndRandom)
		SHADER_PARAMETER(float, Seed)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, RWChunksToDisableCount)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, RWChunksToDisable)
		

	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		/*
		* Here you define constants that can be used statically in the shader code.
		* Example:
		*/
		// OutEnvironment.SetDefine(TEXT("MY_CUSTOM_CONST"), TEXT("1"));

		/*
		* These defines are used in the thread count section of our shader
		*/
		OutEnvironment.SetDefine(TEXT("THREADS_X_SHADER1"), NUM_THREADS_PivotPainterDestroyCompute_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y_SHADER1"), NUM_THREADS_PivotPainterDestroyCompute_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z_SHADER1"), NUM_THREADS_PivotPainterDestroyCompute_Z);

		// This shader must support typed UAV load and we are testing if it is supported at runtime using RHIIsTypedUAVLoadSupported
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);

		// FForwardLightingParameters::ModifyCompilationEnvironment(Parameters.Platform, OutEnvironment);
	}
private:
};
class PIVOTPAINTERDESTROYMODULE_API FPivotPainterDestroyAnimateChunksByHitPoint: public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FPivotPainterDestroyAnimateChunksByHitPoint);
	SHADER_USE_PARAMETER_STRUCT(FPivotPainterDestroyAnimateChunksByHitPoint, FGlobalShader);
	
	
	class FFPivotPainterDestroyAnimateChunksByHitPoint_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<
		FFPivotPainterDestroyAnimateChunksByHitPoint_Perm_TEST
	>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/*
		* Here's where you define one or more of the input parameters for your shader.
		* Some examples:
		*/
		// SHADER_PARAMETER(uint32, MyUint32) // On the shader side: uint32 MyUint32;
		// SHADER_PARAMETER(FVector3f, MyVector) // On the shader side: float3 MyVector;

		// SHADER_PARAMETER_TEXTURE(Texture2D, MyTexture) // On the shader side: Texture2D<float4> MyTexture; (float4 should be whatever you expect each pixel in the texture to be, in this case float4(R,G,B,A) for 4 channels)
		// SHADER_PARAMETER_SAMPLER(SamplerState, MyTextureSampler) // On the shader side: SamplerState MySampler; // CPP side: TStaticSamplerState<ESamplerFilter::SF_Bilinear>::GetRHI();

		// SHADER_PARAMETER_ARRAY(float, MyFloatArray, [3]) // On the shader side: float MyFloatArray[3];

		// SHADER_PARAMETER_UAV(RWTexture2D<FVector4f>, MyTextureUAV) // On the shader side: RWTexture2D<float4> MyTextureUAV;
		// SHADER_PARAMETER_UAV(RWStructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWStructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_UAV(RWBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWBuffer<FMyCustomStruct> MyCustomStructs;

		// SHADER_PARAMETER_SRV(StructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: StructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Buffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: Buffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Texture2D<FVector4f>, MyReadOnlyTexture) // On the shader side: Texture2D<float4> MyReadOnlyTexture;

		// SHADER_PARAMETER_STRUCT_REF(FMyCustomStruct, MyCustomStruct)

		
		SHADER_PARAMETER(float, Gametime)
		SHADER_PARAMETER(float, InverseSpreadDistance)
		SHADER_PARAMETER(float, MaxDelay)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture<float>, RWTimestamp)
		SHADER_PARAMETER(FVector3f, InputHitPoint)
		SHADER_PARAMETER(FVector3f, RayDirection)
		SHADER_PARAMETER(FMatrix44f, ObjectMatrix)
		SHADER_PARAMETER_TEXTURE(Texture2D, PivotPainterOriginsExtent)
		SHADER_PARAMETER_TEXTURE(Texture2D, Timestamp)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, ClosestHit)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(Texture2D, RWNormalAndRandom)
		SHADER_PARAMETER(float, Seed)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, RWChunksToDisableCount)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, RWChunksToDisable)
		

	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		/*
		* Here you define constants that can be used statically in the shader code.
		* Example:
		*/
		// OutEnvironment.SetDefine(TEXT("MY_CUSTOM_CONST"), TEXT("1"));

		/*
		* These defines are used in the thread count section of our shader
		*/
		OutEnvironment.SetDefine(TEXT("THREADS_X_SHADER2"), NUM_THREADS_PivotPainterDestroyCompute_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y_SHADER2"), NUM_THREADS_PivotPainterDestroyCompute_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z_SHADER2"), NUM_THREADS_PivotPainterDestroyCompute_Z);

		// This shader must support typed UAV load and we are testing if it is supported at runtime using RHIIsTypedUAVLoadSupported
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);

		// FForwardLightingParameters::ModifyCompilationEnvironment(Parameters.Platform, OutEnvironment);
	}
private:
};

// This will tell the engine to create the shader and where the shader entry point is.
//                            ShaderType                            ShaderPath                     Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FPivotPainterDestroyFindClosestHit, "/PivotPainterDestroyModuleShaders/PivotPainterDestroyCompute/PivotPainterDestroyCompute.usf", "PivotPainterDestroyFindClosestHit", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FPivotPainterDestroyAnimateChunks, "/PivotPainterDestroyModuleShaders/PivotPainterDestroyCompute/PivotPainterDestroyCompute.usf", "PivotPainterDestroyAnimateChunks", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FPivotPainterDestroyResetBuffers, "/PivotPainterDestroyModuleShaders/PivotPainterDestroyCompute/PivotPainterDestroyCompute.usf", "PivotPainterDestroyResetBuffers", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FPivotPainterDestroyAnimateChunksByHitPoint, "/PivotPainterDestroyModuleShaders/PivotPainterDestroyCompute/PivotPainterDestroyCompute.usf", "PivotPainterDestroyAnimateChunksByHitPoint", SF_Compute);
void FPivotPainterDestroyComputeInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FPivotPainterDestroyComputeDispatchParams Params, TFunction<void(FOutStruct OutStruct)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);

	{
		SCOPE_CYCLE_COUNTER(STAT_PivotPainterDestroyCompute_Execute);
		DECLARE_GPU_STAT(PivotPainterDestroyCompute)
		RDG_EVENT_SCOPE(GraphBuilder, "PivotPainterDestroyCompute");
		RDG_GPU_STAT_SCOPE(GraphBuilder, PivotPainterDestroyCompute);
		
		typename FPivotPainterDestroyFindClosestHit::FPermutationDomain FindClosestHitPermutationVector;
		TShaderMapRef<FPivotPainterDestroyFindClosestHit> ComputeShaderFindClosestHit(GetGlobalShaderMap(GMaxRHIFeatureLevel), FindClosestHitPermutationVector);
		
		typename FPivotPainterDestroyAnimateChunks::FPermutationDomain AnimateChunksPermutationVector;
		TShaderMapRef<FPivotPainterDestroyAnimateChunks> ComputeShaderAnimateChunks(GetGlobalShaderMap(GMaxRHIFeatureLevel), AnimateChunksPermutationVector);
		
		typename FPivotPainterDestroyResetBuffers::FPermutationDomain ResetBuffersPermutationVector;
		TShaderMapRef<FPivotPainterDestroyResetBuffers> ComputeShaderResetBuffers(GetGlobalShaderMap(GMaxRHIFeatureLevel), ResetBuffersPermutationVector);
		
		typename FPivotPainterDestroyAnimateChunksByHitPoint::FPermutationDomain PermutationVectorAnimateChunksByHitPoint;
		TShaderMapRef<FPivotPainterDestroyAnimateChunksByHitPoint> ComputeShaderAnimateChunksByHitPoint(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVectorAnimateChunksByHitPoint);

		bool bIsShader0Valid = ComputeShaderFindClosestHit.IsValid();
		bool bIsShader1Valid = ComputeShaderAnimateChunks.IsValid();
		bool bIsShader2Valid = ComputeShaderResetBuffers.IsValid();
		bool bIsShader3Valid = ComputeShaderAnimateChunksByHitPoint.IsValid();

		if (bIsShader0Valid && bIsShader1Valid && bIsShader2Valid && bIsShader3Valid) {
			const bool HitVariant = Params.BAssignByHitPoint;
		
			const EPixelFormat TimestampFormat = Params.Timestamp->GetRenderTargetTexture()->GetFormat();
			FRDGTextureDesc TimestampTextureDesc(FRDGTextureDesc::Create2D(Params.Timestamp->GetSizeXY(),TimestampFormat,FClearValueBinding::Black,TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV));
			FRDGTextureRef TmpTimestamp = GraphBuilder.CreateTexture(TimestampTextureDesc,TEXT("TmpTimestamp"));
			FRDGTextureRef TimestampTargetTexture = RegisterExternalTexture(GraphBuilder,Params.Timestamp->GetRenderTargetTexture(),TEXT("TimestampTargetTexture"));

			const EPixelFormat NormalAndRandomFormat = Params.NormalAndRandom->GetRenderTargetTexture()->GetFormat();
			FRDGTextureDesc NormalAndRandomTextureDesc(FRDGTextureDesc::Create2D(Params.NormalAndRandom->GetSizeXY(), NormalAndRandomFormat, FClearValueBinding::Transparent, TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV));
			FRDGTextureRef TmpNormalAndRandom= GraphBuilder.CreateTexture(NormalAndRandomTextureDesc, TEXT("TmpNormalAndRandom"));
			FRDGTextureRef NormalAndRandomTexture = RegisterExternalTexture(GraphBuilder, Params.NormalAndRandom->GetRenderTargetTexture(), TEXT("Timestamp_RT"));

			auto RWTmpNormalAndRandom=  GraphBuilder.CreateUAV(TmpNormalAndRandom);

			
			auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z),FIntVector(NUM_THREADS_PivotPainterDestroyCompute_X, NUM_THREADS_PivotPainterDestroyCompute_Y, NUM_THREADS_PivotPainterDestroyCompute_Z));
			AddCopyTexturePass(GraphBuilder, TimestampTargetTexture, TmpTimestamp, FRHICopyTextureInfo());
			FRDGBufferRef RWClosestHitRef;
			FRDGBufferUAVRef RWClosestHitUAVRef;
			RWClosestHitRef = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), 1),TEXT("RWClosestHit"));
			RWClosestHitUAVRef = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(RWClosestHitRef, PF_R32_UINT));

			FRDGBufferRef ClosestHitRef;
			FRDGBufferSRVRef ClosestHitSRVRef;
			ClosestHitRef  =  CreateUploadBuffer(GraphBuilder, TEXT("ClosestHit"), sizeof(uint32), 1,nullptr, sizeof(uint32));
			ClosestHitSRVRef = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(ClosestHitRef, PF_R32_UINT));

			FRDGBufferRef RWChunksToDisableCountRef  = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), 1),TEXT("RWChunksToDisableCount"));
			FRDGBufferUAVRef RWChunksToDisableCountUAVRef = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(RWChunksToDisableCountRef, PF_R32_UINT));

			FRDGBufferRef RWChunksToDisableRef  = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), MaxChunksToDisable),TEXT("RWChunksToDisableCount"));
			FRDGBufferUAVRef RWChunksToDisableUAVRef = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(RWChunksToDisableRef, PF_R32_UINT));

			FPivotPainterDestroyResetBuffers::FParameters* PassParametersResetBuffers = GraphBuilder.AllocParameters<FPivotPainterDestroyResetBuffers::FParameters>();
			PassParametersResetBuffers->RWClosestHit = RWClosestHitUAVRef;
			PassParametersResetBuffers->RWChunksToDisableCount = RWChunksToDisableCountUAVRef;

			GraphBuilder.AddPass(
			RDG_EVENT_NAME("ResetBuffers"),
			PassParametersResetBuffers,
			ERDGPassFlags::AsyncCompute,
			[&PassParametersResetBuffers, ComputeShaderResetBuffers, GroupCount](FRHIComputeCommandList& RHICmdList)
		{
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShaderResetBuffers, *PassParametersResetBuffers, FIntVector(1, 1,1));
		});
			if(!HitVariant)
			{
				FPivotPainterDestroyFindClosestHit::FParameters* FindClosestHitPassParameters = GraphBuilder.AllocParameters<FPivotPainterDestroyFindClosestHit::FParameters>();
				FindClosestHitPassParameters->Gametime = Params.Gametime;
				FindClosestHitPassParameters->RayOrigin = Params.RayOrigin;
				FindClosestHitPassParameters->RayDirection = Params.RayDirection;
				FindClosestHitPassParameters->RayOrigin = Params.RayOrigin;
				FindClosestHitPassParameters->ObjectMatrix = Params.ObjectMatrix;
				FindClosestHitPassParameters->PivotPainterOriginsExtent = Params.PivotPainterOriginExtent->GetResource()->TextureRHI->GetTexture2D();
				FindClosestHitPassParameters->Timestamp = Params.Timestamp->GetRenderTargetTexture();
				FindClosestHitPassParameters->RWClosestHit = RWClosestHitUAVRef;
				GraphBuilder.AddPass(
					RDG_EVENT_NAME("ExecutePivotPainterDestroyCompute"),
					FindClosestHitPassParameters,
					ERDGPassFlags::AsyncCompute,
					[&FindClosestHitPassParameters, ComputeShaderFindClosestHit, GroupCount](FRHIComputeCommandList& RHICmdList)
				{
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShaderFindClosestHit, *FindClosestHitPassParameters, GroupCount);
				});
				AddCopyBufferPass(GraphBuilder, ClosestHitRef, 0, RWClosestHitRef, 0,sizeof(uint32));
				FPivotPainterDestroyAnimateChunks::FParameters* AnimateChunksPassParameters = GraphBuilder.AllocParameters<FPivotPainterDestroyAnimateChunks::FParameters>();
				AnimateChunksPassParameters->ClosestHit = ClosestHitSRVRef;
				AnimateChunksPassParameters->RWTimestamp = GraphBuilder.CreateUAV(TmpTimestamp);
				AnimateChunksPassParameters->Gametime = Params.Gametime;
				AnimateChunksPassParameters->InverseSpreadDistance = Params.InverseSpreadDistance;
				AnimateChunksPassParameters->MaxDelay = Params.MaxDelay;
				AnimateChunksPassParameters->RayOrigin = Params.RayOrigin;
				AnimateChunksPassParameters->RayDirection = Params.RayDirection;
				AnimateChunksPassParameters->ObjectMatrix = Params.ObjectMatrix;
				AnimateChunksPassParameters->PivotPainterOriginsExtent = Params.PivotPainterOriginExtent->GetResource()->TextureRHI->GetTexture2D();
				AnimateChunksPassParameters->Timestamp = Params.Timestamp->GetRenderTargetTexture();
				AnimateChunksPassParameters->RWNormalAndRandom = RWTmpNormalAndRandom;
				AnimateChunksPassParameters->RWChunksToDisableCount = RWChunksToDisableCountUAVRef;
				AnimateChunksPassParameters->RWChunksToDisable = RWChunksToDisableUAVRef;
				GraphBuilder.AddPass(
					RDG_EVENT_NAME("ExecutePivotPainterDestroyComputeAnimateChunks"),
					AnimateChunksPassParameters,
					ERDGPassFlags::AsyncCompute,
					[&AnimateChunksPassParameters, ComputeShaderAnimateChunks, GroupCount](FRHIComputeCommandList& RHICmdList)
				{
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShaderAnimateChunks, *AnimateChunksPassParameters, GroupCount);
				});
			}
			else
			{
				FPivotPainterDestroyAnimateChunksByHitPoint::FParameters* AnimateChunksPassParametersByHitPoint = GraphBuilder.AllocParameters<FPivotPainterDestroyAnimateChunksByHitPoint::FParameters>();
				AnimateChunksPassParametersByHitPoint->InputHitPoint = Params.InputHitPoint;
				AnimateChunksPassParametersByHitPoint->RayDirection = Params.RayDirection;
				AnimateChunksPassParametersByHitPoint->ObjectMatrix = Params.ObjectMatrix;
				AnimateChunksPassParametersByHitPoint->PivotPainterOriginsExtent = Params.PivotPainterOriginExtent->GetResource()->TextureRHI->GetTexture2D();
				AnimateChunksPassParametersByHitPoint->Timestamp =  Params.Timestamp->GetRenderTargetTexture();
				AnimateChunksPassParametersByHitPoint->RWTimestamp =  GraphBuilder.CreateUAV(TmpTimestamp);
				AnimateChunksPassParametersByHitPoint->RWNormalAndRandom = RWTmpNormalAndRandom;
				AnimateChunksPassParametersByHitPoint->Gametime = Params.Gametime;
				AnimateChunksPassParametersByHitPoint->MaxDelay = Params.MaxDelay;
				AnimateChunksPassParametersByHitPoint->InverseSpreadDistance = Params.InverseSpreadDistance;
				AnimateChunksPassParametersByHitPoint->Seed = Params.Seed;
				AnimateChunksPassParametersByHitPoint->RWChunksToDisableCount = RWChunksToDisableCountUAVRef;
				AnimateChunksPassParametersByHitPoint->RWChunksToDisable = RWChunksToDisableUAVRef;
				GraphBuilder.AddPass(
					RDG_EVENT_NAME("ExecuteAnimateChunksByHit"),
					AnimateChunksPassParametersByHitPoint,
					ERDGPassFlags::AsyncCompute,
					[&AnimateChunksPassParametersByHitPoint, ComputeShaderAnimateChunksByHitPoint, GroupCount](FRHIComputeCommandList& RHICmdList)
				{
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShaderAnimateChunksByHitPoint, *AnimateChunksPassParametersByHitPoint, GroupCount);
				});
			}
			
			FRHIGPUBufferReadback* GPUBufferReadbackChunksCount = new FRHIGPUBufferReadback(TEXT("GPUBufferReadbackChunksCount"));
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadbackChunksCount, RWChunksToDisableCountRef,sizeof(uint32));

			FRHIGPUBufferReadback* GPUBufferReadbackChunksIndexes= new FRHIGPUBufferReadback(TEXT("GPUBufferReadbackChunksIndexes"));
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadbackChunksIndexes, RWChunksToDisableRef,sizeof(uint32)*MaxChunksToDisable);

			FRHIGPUBufferReadback* GPUBufferReadbackClosestHit = new FRHIGPUBufferReadback(TEXT("GPUBufferReadbackClosestHit"));
			if(!HitVariant)
			{
				AddEnqueueCopyPass(GraphBuilder, GPUBufferReadbackClosestHit, RWClosestHitRef,sizeof(uint32));
			}
			
			AddCopyTexturePass(GraphBuilder, TmpTimestamp, TimestampTargetTexture, FRHICopyTextureInfo());
			AddCopyTexturePass(GraphBuilder, TmpNormalAndRandom, NormalAndRandomTexture, FRHICopyTextureInfo());

			auto RunnerFunc = [ GPUBufferReadbackChunksCount,GPUBufferReadbackChunksIndexes,GPUBufferReadbackClosestHit,Params,HitVariant,AsyncCallback](auto&& RunnerFunc) -> void {
				 if (GPUBufferReadbackChunksCount->IsReady() && GPUBufferReadbackChunksIndexes->IsReady()&& (GPUBufferReadbackClosestHit->IsReady() || HitVariant)) {
				 	FOutStruct OutStruct;
				 	const float RayLenght = Params.RayDirection.Size();
					const uint32* CounterBuffer = static_cast<uint32*>(GPUBufferReadbackChunksCount->Lock(sizeof(uint32)));
					const uint32 Counter = CounterBuffer[0];
				 	
				 	const uint32* ChunksIndexes =  static_cast<uint32*>(GPUBufferReadbackChunksIndexes->Lock(sizeof(uint32) * (MaxChunksToDisable)));
				 	if(!HitVariant)
				 	{
				 		const uint32* FoundDistance =  static_cast<uint32*>(GPUBufferReadbackClosestHit->Lock(sizeof(uint32)));
				 		uint32 FoundDistanceBits = FoundDistance[0];
						
	
					
				 		float ReturnDistance = -1.0;
				 		if(FoundDistanceBits > 0)
				 		{
				 			FoundDistanceBits &= 0x007fffff;
				 			FoundDistanceBits |= 0x3f800000;
				 			const float NormalizedFlotPlusOne = *reinterpret_cast<float*>(&FoundDistanceBits);
							
				 			ReturnDistance =  (2.0f- NormalizedFlotPlusOne) * RayLenght;
						
				 		}
				 		OutStruct.HitDistance = ReturnDistance;
					
				 	}
				    else
				    {
				    	OutStruct.HitDistance = RayLenght;
				    }
				 	OutStruct.Count = static_cast<int32>( Counter);
				 	for (uint32 i = 0; i < Counter && i < MaxChunksToDisable; ++i)
				 	{
				 		const uint32 NewElement = ChunksIndexes[i];
				 		const int32 NewElementCast = static_cast<int32>(NewElement);
					
				 		OutStruct.OutIndexes[i] = NewElementCast;
				 	}
				// 	
				 	GPUBufferReadbackChunksCount->Unlock();
				 	GPUBufferReadbackChunksIndexes->Unlock();
				 	if(!HitVariant)
				 		GPUBufferReadbackClosestHit->Unlock();
				//
				 	AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutStruct]() {
						AsyncCallback(OutStruct);
					});
				//
				 	delete GPUBufferReadbackChunksCount;
				 	delete GPUBufferReadbackChunksIndexes;
				 	delete GPUBufferReadbackClosestHit;
				 } else {
					AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
						RunnerFunc(RunnerFunc);
					});
				}
			};

			AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
				RunnerFunc(RunnerFunc);
			});
			
		} else {
			#if WITH_EDITOR
				GEngine->AddOnScreenDebugMessage((uint64)42145125184, 6.f, FColor::Red, FString(TEXT("The compute shader has a problem.")));
			#endif

			// We exit here as we don't want to crash the game if the shader is not found or has an error.
			
		}
	}

	GraphBuilder.Execute();
}