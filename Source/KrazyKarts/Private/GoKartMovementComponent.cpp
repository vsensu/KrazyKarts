// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementComponent.h"

// Sets default values for this component's properties
UGoKartMovementComponent::UGoKartMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	Mass = 1000;
	MaxDrivingForce = 10000;
	MinTurningRadius = 10;
	DragCoefficient = 16;
	RollingCoefficient = 0.015;
}


// Called when the game starts
void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGoKartMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UGoKartMovementComponent::SimulateMove(FGoKartMove Move)
{
	auto Force = GetOwner()->GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	auto a = Force / Mass;
	
	auto dv = a * Move.DeltaTime;
	
	Velocity = Velocity + dv;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);
	
	UpdateLocationFromVelocity(Move.DeltaTime);
}

FGoKartMove UGoKartMovementComponent::CreateMove(float DeltaTime) const
{
	FGoKartMove move;
 	move.DeltaTime = DeltaTime;
 	move.Throttle = Throttle;
 	move.SteeringThrow = SteeringThrow;
	move.Timestamp = GetWorld()->TimeSeconds;

	return move;
}

FVector UGoKartMovementComponent::GetAirResistance() const
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector UGoKartMovementComponent::GetRollingResistance() const
{
	auto g = FMath::Abs(GetWorld()->GetGravityZ() / 100);
	auto normalForce = Mass * g;
	return -Velocity.GetSafeNormal() * RollingCoefficient * normalForce;
}

void UGoKartMovementComponent::ApplyRotation(float DeltaTime, float NewSteeringThrow)
{
	auto dx = FVector::DotProduct(GetOwner()->GetActorForwardVector(), Velocity) * DeltaTime;
	
	auto angleDelta = dx / MinTurningRadius * NewSteeringThrow;

	FQuat dr(GetOwner()->GetActorUpVector(), angleDelta);

	GetOwner()->AddActorWorldRotation(dr);

	Velocity = dr.RotateVector(Velocity);
}

void UGoKartMovementComponent::UpdateLocationFromVelocity(float DeltaTime)
{
	auto translation = Velocity * 100 * DeltaTime;

	FHitResult hit;
	GetOwner()->AddActorWorldOffset(translation, true, &hit);

	if(hit.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}
