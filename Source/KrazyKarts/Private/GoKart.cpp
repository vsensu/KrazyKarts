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

	MovementComp = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("MovementComp"));

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
		auto move = MovementComp->CreateMove(DeltaTime);
		UnacknowledgedMoves.Add(move);
		MovementComp->SimulateMove(move);
		Server_SendMove(move);
	}
	else if(GetLocalRole() == ROLE_Authority && GetRemoteRole() == ROLE_SimulatedProxy)
	{
		// server and local control

		auto move = MovementComp->CreateMove(DeltaTime);
		Server_SendMove(move);
	}
	else if(GetLocalRole() == ROLE_SimulatedProxy)
	{
		MovementComp->SimulateMove(ServerState.LastMove);
	}
	
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this, FColor::White, 0);
}

void AGoKart::OnRep_ServerState()
{
	SetActorTransform(ServerState.Transform);
	MovementComp->SetVelocity(ServerState.Velocity);

	ClearAcknowledgedMoves(ServerState.LastMove);
	for(const auto &move : UnacknowledgedMoves)
	{
		MovementComp->SimulateMove(move);
	}
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
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

void AGoKart::MoveForward(float Val)
{
	MovementComp->SetThrottle(Val);
}

void AGoKart::MoveRight(float Val)
{
	MovementComp->SetSteeringThrow(Val);
}

void AGoKart::Server_SendMove_Implementation(FGoKartMove Val)
{
	MovementComp->SimulateMove(Val);

	ServerState.LastMove = Val;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = MovementComp->GetVelocity();
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Val)
{
	// TODO: validate
	return true;
}
