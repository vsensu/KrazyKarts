// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Mass = 1000;
	MaxDrivingForce = 10000;
	MinTurningRadius = 10;
	DragCoefficient = 16;
	RollingCoefficient = 0.015;
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	if(HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
	
}

void AGoKart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGoKart, ServerState);
}

FString GetEnumText(ENetRole role)
{
	switch (role)
	{
	case ROLE_None: return "None";
	case ROLE_SimulatedProxy: return "SimulatedProxy";
	case ROLE_AutonomousProxy: return "AutonomousProxy";
	case ROLE_Authority: return "Authority";
	default: return "Error";
	}
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(GetLocalRole() == ROLE_AutonomousProxy)
	{
		auto move = CreateMove(DeltaTime);
		UnacknowledgedMoves.Add(move);
		SimulateMove(move);
		Server_SendMove(move);
	}
	else if(GetLocalRole() == ROLE_Authority && GetRemoteRole() == ROLE_SimulatedProxy)
	{
		// server and local control

		auto move = CreateMove(DeltaTime);
		Server_SendMove(move);
	}
	else if(GetLocalRole() == ROLE_SimulatedProxy)
	{
		SimulateMove(ServerState.LastMove);
	}
	
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this, FColor::White, 0);
}

void AGoKart::OnRep_ServerState()
{
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;

	ClearAcknowledgedMoves(ServerState.LastMove);
	for(const auto &move : UnacknowledgedMoves)
	{
		SimulateMove(move);
	}
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

void AGoKart::SimulateMove(FGoKartMove Move)
{
	auto Force = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	auto a = Force / Mass;
	
	auto dv = a * Move.DeltaTime;
	
	Velocity = Velocity + dv;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);
	
	UpdateLocationFromVelocity(Move.DeltaTime);
}

FGoKartMove AGoKart::CreateMove(float DeltaTime)
{
	FGoKartMove move;
 	move.DeltaTime = DeltaTime;
 	move.Throttle = Throttle;
 	move.SteeringThrow = SteeringThrow;
	move.Timestamp = GetWorld()->TimeSeconds;

	return move;
}

void AGoKart::ClearAcknowledgedMoves(FGoKartMove LastMove)
{
	decltype(UnacknowledgedMoves) NewMoves;
	for(const auto &move : UnacknowledgedMoves)
	{
		if(move.Timestamp > LastMove.Timestamp)
		{
			NewMoves.Add(move);
		}
	}

	UnacknowledgedMoves = NewMoves;
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

void AGoKart::Server_SendMove_Implementation(FGoKartMove Val)
{
	SimulateMove(Val);

	ServerState.LastMove = Val;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = Velocity;
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Val)
{
	// TODO: validate
	return true;
}

void AGoKart::ApplyRotation(float DeltaTime, float NewSteeringThrow)
{
	auto dx = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	
	auto angleDelta = dx / MinTurningRadius * NewSteeringThrow;

	FQuat dr(GetActorUpVector(), angleDelta);

	AddActorWorldRotation(dr);

	Velocity = dr.RotateVector(Velocity);
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
