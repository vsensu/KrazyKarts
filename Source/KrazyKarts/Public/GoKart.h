// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKartMovementComponent.h"
#include "GoKart.generated.h"


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


UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void ClearAcknowledgedMoves(FGoKartMove LastMove);

	void MoveForward(float Val);
	void MoveRight(float Val);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Val);
	
	UFUNCTION()
    void OnRep_ServerState();

	UPROPERTY(ReplicatedUsing=OnRep_ServerState)
	FGoKartState ServerState;

	UPROPERTY()
	UGoKartMovementComponent *MovementComp;

	TArray<FGoKartMove> UnacknowledgedMoves;
};
