// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"


USTRUCT()
struct FGoKartMove
{
	GENERATED_BODY()

    UPROPERTY()
    float SteeringThrow;

	UPROPERTY()
    float Throttle;

	UPROPERTY()
    float DeltaTime;
	
	UPROPERTY()
    float Timestamp;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SimulateMove(FGoKartMove Move);

	FVector GetVelocity() const {return Velocity;}
	void SetVelocity(const FVector &Val) {Velocity = Val;}

	void SetThrottle(float Val) {Throttle = Val;}
	void SetSteeringThrow(float Val) {SteeringThrow = Val;}

	FGoKartMove GetLastMove() const {return LastMove;}

private:
	FGoKartMove CreateMove(float DeltaTime) const;
	void ApplyRotation(float DeltaTime, float NewSteeringThrow);
	void UpdateLocationFromVelocity(float DeltaTime);
	FVector GetAirResistance() const;
	FVector GetRollingResistance() const;

	// The mass of the car (kg).
	UPROPERTY(EditAnywhere)
    float Mass;

	// The force applied to the car when the throttle is fully down (N).
	UPROPERTY(EditAnywhere)
    float MaxDrivingForce;

	UPROPERTY(EditAnywhere)
    float MinTurningRadius;

	UPROPERTY(EditAnywhere)
    float DragCoefficient;

	UPROPERTY(EditAnywhere)
    float RollingCoefficient;
	
	float SteeringThrow;
	FVector Velocity;
	float Throttle;

	FGoKartMove LastMove;
		
};
