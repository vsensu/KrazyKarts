// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mass = 1000;
	MaxDrivingForce = 10000;
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

	auto a = Force / Mass;
	
	auto dv = a * DeltaTime;
	
	Velocity = Velocity + dv;
	
	auto translation = Velocity * 100 * DeltaTime;
	AddActorWorldOffset(translation);
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
}

void AGoKart::MoveForward(float Val)
{
	Throttle = Val;
}

