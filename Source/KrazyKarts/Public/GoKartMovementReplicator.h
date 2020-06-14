// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicator.generated.h"


USTRUCT()
struct FGoKartState 
{
	GENERATED_BODY()

    UPROPERTY()
    FTransform Transform;

	UPROPERTY()
    FVector Velocity;

	UPROPERTY()
    FGoKartMove LastMove;
};


struct FHermiteCubicSpline
{
	FVector StartLocation, StartDerivative, TargetLocation, TargetDerivative;

	FVector Interpolate(float ratio) const
	{
		return FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, ratio);
	}

	FVector InterpolateDerivative(float ratio) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, ratio);
	}
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementReplicator : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementReplicator();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void SimulatedProxyTick(float DeltaTime);
	FHermiteCubicSpline CreateSpline() const;

	void InterpolateLocation(const FHermiteCubicSpline &Spline, float LerpRatio);
	void InterpolateVelocity(const FHermiteCubicSpline &Spline, float LerpRatio);
	void InterpolateRotation(float LerpRatio);
	
	void ClearAcknowledgedMoves(FGoKartMove LastMove);
	
	UFUNCTION(Server, Reliable, WithValidation)
    void Server_SendMove(FGoKartMove Val);
	
	UFUNCTION()
    void OnRep_ServerState();

	void AutonomousProxy_OnRep_ServerState();
	void SimulatedProxy_OnRep_ServerState();

	void UpdateServerState(const FGoKartMove &Val);

	UPROPERTY(ReplicatedUsing=OnRep_ServerState)
    FGoKartState ServerState;
	
	TArray<FGoKartMove> UnacknowledgedMoves;

	UPROPERTY()
	class UGoKartMovementComponent *MovementComp;

	UPROPERTY()
	USceneComponent *MeshOffsetRoot;

	UFUNCTION(BlueprintCallable)
	void SetMeshOffsetRoot(USceneComponent *Root) {MeshOffsetRoot = Root;}

	float SimulatedProxyTimeSinceLastUpdate;
	float SimulatedProxyTimeBetweenLastUpdates;

	FTransform SimulatedProxyStartTransform;
	FVector SimulatedProxyStartVelocity;
};
