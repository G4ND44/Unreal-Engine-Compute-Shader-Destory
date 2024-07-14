#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialRenderProxy.h"

#include "PivotPainterDestroyCompute.generated.h"
constexpr int32 MaxChunksToDisable = 256;

struct PIVOTPAINTERDESTROYMODULE_API FPivotPainterDestroyComputeDispatchParams
{
	int X;
	int Y;
	int Z;
	FRenderTarget* Timestamp;
	float Gametime;
	float InverseSpreadDistance;
	float MaxDelay;
	float Seed;
	
	FVector3f RayOrigin;
	FVector3f RayDirection;
	FVector3f InputHitPoint;
	bool BAssignByHitPoint;
	FMatrix44f ObjectMatrix;
	UTexture2D* PivotPainterOriginExtent;
	FRenderTarget* NormalAndRandom;

	FPivotPainterDestroyComputeDispatchParams(int x, int y, int z)
		: X(x)
		  , Y(y)
		  , Z(z)
	{
	}
};

USTRUCT(Blueprintable)
struct FOutStruct
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly)
	int32 Count;
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> OutIndexes;
	UPROPERTY(BlueprintReadOnly)
	float HitDistance;
	FOutStruct()
	{
		OutIndexes.Init(0, (MaxChunksToDisable));
		Count = 0;
		HitDistance = -1.0f;
	}
};

// This is a public interface that we define so outside code can invoke our compute shader.
class PIVOTPAINTERDESTROYMODULE_API FPivotPainterDestroyComputeInterface
{
public:
	// Executes this shader on the render thread
	static void DispatchRenderThread(
		FRHICommandListImmediate& RHICmdList,
		FPivotPainterDestroyComputeDispatchParams Params,
		TFunction<void(FOutStruct OutStruct)> AsyncCallback
	);

	// Executes this shader on the render thread from the game thread via EnqueueRenderThreadCommand
	static void DispatchGameThread(
		FPivotPainterDestroyComputeDispatchParams Params,
		TFunction<void(FOutStruct OutStruct)> AsyncCallback
	)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
			{
				DispatchRenderThread(RHICmdList, Params, AsyncCallback);
			});
	}

	// Dispatches this shader. Can be called from any thread
	static void Dispatch(
		FPivotPainterDestroyComputeDispatchParams Params,
		TFunction<void(FOutStruct OutStruct)> AsyncCallback
	)
	{
		if (IsInRenderingThread())
		{
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		}
		else
		{
			DispatchGameThread(Params, AsyncCallback);
		}
	}

	static void CopyTextureToRenderTarget(
		UTexture2D* Input,
		FRenderTarget* Target,
		FRHICommandListImmediate& RHICmdList
	);
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPivotPainterDestroyComputeLibrary_AsyncExecutionCompleted, FOutStruct,
                                            OutStruct);


UCLASS() // Change the _API to match your project
class PIVOTPAINTERDESTROYMODULE_API UPivotPainterDestroyComputeLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UTextureRenderTarget2D* Timestamp;
	UTextureRenderTarget2D* NormalAndRandomResult;
	float Gametime;
	float InverseSpreadDistance;
	float MaxDelay;
	FVector3f RayOrigin;
	FVector3f RayDirection;
	FVector3f InputHitPoint;
	bool BAssignByHitPoint;
	FMatrix44f ObjectMatrix;
	UTexture2D* PivotPainterOriginExtent;
	// Execute the actual load
	virtual void Activate() override
	{
		// Create a dispatch parameters struct and set our desired seed
		FPivotPainterDestroyComputeDispatchParams Params(Timestamp->SizeX, Timestamp->SizeY, 1);
		Params.Timestamp = Timestamp->GameThread_GetRenderTargetResource();
		Params.NormalAndRandom = NormalAndRandomResult->GameThread_GetRenderTargetResource();
		Params.Gametime = Gametime;
		Params.RayOrigin = RayOrigin;
		Params.RayDirection = RayDirection;
		Params.ObjectMatrix = ObjectMatrix;
		Params.PivotPainterOriginExtent = PivotPainterOriginExtent;
		Params.MaxDelay = MaxDelay;
		Params.InverseSpreadDistance = InverseSpreadDistance;
		Params.Seed = FMath::FRand();
		Params.BAssignByHitPoint = BAssignByHitPoint;
		Params.InputHitPoint = InputHitPoint;
		// Dispatch the compute shader and wait until it completes
		FPivotPainterDestroyComputeInterface::Dispatch(Params, [this](FOutStruct OutStruct)
		{
			Completed.Broadcast(OutStruct);
		});
	}
	static UPivotPainterDestroyComputeLibrary_AsyncExecution* ExecutePivotDestroyComputeShaderHelper(
		UObject* WorldContextObject, const float SpreadDistance,
		const FMatrix44f ObjectMatrix, UTexture2D* PivotPainterOriginsExtent, UTextureRenderTarget2D* Timestamp,
		UTextureRenderTarget2D* NormalAndRandomResult, const float Gametime, const float MaxDelay,
		const bool BAssignByHitPoint, const FVector3f RayStart, const FVector3f RayEnd,
		const FVector3f InputHitPoint = FVector3f::Zero())
	{
		//if(BIsWorking)
		//	return nullptr;
		UPivotPainterDestroyComputeLibrary_AsyncExecution* Action = NewObject<
			UPivotPainterDestroyComputeLibrary_AsyncExecution>();

		Action->RayOrigin = RayStart;
		FVector3f DirectionVector = RayEnd - RayStart;

		Action->RayDirection = DirectionVector;
	
		Action->ObjectMatrix = ObjectMatrix;
		Action->PivotPainterOriginExtent = PivotPainterOriginsExtent;
		Action->Timestamp = Timestamp;
		Action->NormalAndRandomResult = NormalAndRandomResult;
		Action->Gametime = Gametime;
		Action->InverseSpreadDistance = 1.0f / SpreadDistance;
		Action->MaxDelay = MaxDelay;
		Action->InputHitPoint = InputHitPoint;
		Action->BAssignByHitPoint = BAssignByHitPoint;
		Action->RegisterWithGameInstance(WorldContextObject);
		//BIsWorking = true;
		return Action;
	}
	UFUNCTION(BlueprintCallable,
		meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UPivotPainterDestroyComputeLibrary_AsyncExecution* ExecutePivotPainterDestroy(
		UObject* WorldContextObject, const FVector3f RayOrigin, const FVector3f RayEnd, const float SpreadDistance,
		const float MaxDelay, const FMatrix44f ObjectMatrix, UTexture2D* PivotPainterOriginExtent,
		UTextureRenderTarget2D* Timestamp, UTextureRenderTarget2D* NormalAndRandomResult, float Gametime)
	{
		return ExecutePivotDestroyComputeShaderHelper(WorldContextObject,SpreadDistance,ObjectMatrix,PivotPainterOriginExtent,Timestamp,NormalAndRandomResult,Gametime,MaxDelay,false,RayOrigin,RayEnd);
	}

	UFUNCTION(BlueprintCallable,
		meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UPivotPainterDestroyComputeLibrary_AsyncExecution* ExecutePivotPainterDestroyByHit(
		UObject* WorldContextObject, const FVector3f RayOrigin, const FVector3f RayEnd, const float SpreadDistance,
		const float MaxDelay, const FMatrix44f ObjectMatrix, UTexture2D* PivotPainterOriginExtent,
		UTextureRenderTarget2D* Timestamp, UTextureRenderTarget2D* NormalAndRandomResult, float Gametime,const FVector3f InputHitPoint)
	{
		return ExecutePivotDestroyComputeShaderHelper(WorldContextObject,SpreadDistance,ObjectMatrix,PivotPainterOriginExtent,Timestamp,NormalAndRandomResult,Gametime,MaxDelay,true,RayOrigin,RayEnd,InputHitPoint);
	}

	UPROPERTY(BlueprintAssignable)
	FOnPivotPainterDestroyComputeLibrary_AsyncExecutionCompleted Completed;

	UFUNCTION(BlueprintCallable)
	static void CopyTextureToRenderTarget(UTexture2D* Input, UTextureRenderTarget2D* Target)
	{
		FRenderTarget* FTarget = Target->GameThread_GetRenderTargetResource();
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Input, FTarget](FRHICommandListImmediate& RHICmdList)
			{
				FPivotPainterDestroyComputeInterface::CopyTextureToRenderTarget(Input, FTarget, RHICmdList);
			});
	}
};
