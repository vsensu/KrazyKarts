// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mass = 1000;
	MaxDrivingForce = 10000;
	MaxDegreesPerSecond = 90;
	DragCoefficient = 16;
	RollingCoefficient = 0.015;
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	auto Force = GetActorForwardVector() * MaxDrivingForce * Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	auto a = Force / Mass;
	
	auto dv = a * DeltaTime;
	
	Velocity = Velocity + dv;

	auto angleDelta = MaxDegreesPerSecond * DeltaTime * SteeringThrow;

	FQuat dr(GetActorUpVector(), FMath::DegreesToRadians(angleDelta));

	AddActorWorldRotation(dr);

	Velocity = dr.RotateVector(Velocity);
	
	UpdateLocationFromVelocity(DeltaTime);
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

FVector AGoKart::GetAirResistance()
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AGoKart::GetRollingResistance()
{
	auto g = FMath::Abs(GetWorld()->GetGravityZ() / 100);
	auto normalForce = Mass * g;
	return -Velocity.GetSafeNormal() * RollingCoefficient * normalForce;
}

void AGoKart::MoveForward(float Val)
{
	Throttle = Val;
}

void AGoKart::MoveRight(float Val)
{
	SteeringThrow = Val;
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	auto translation = Velocity * 100 * DeltaTime;

	FHitResult hit;
	AddActorWorldOffset(translation, true, &hit);

	if(hit.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}
