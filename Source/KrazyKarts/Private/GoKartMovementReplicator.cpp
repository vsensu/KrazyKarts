// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementReplicator.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UGoKartMovementReplicator::UGoKartMovementReplicator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}


// Called when the game starts
void UGoKartMovementReplicator::BeginPlay()
{
	Super::BeginPlay();

	MovementComp = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
	
}


// Called every frame
void UGoKartMovementReplicator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(MovementComp == nullptr) return;

	if(GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		auto move = MovementComp->CreateMove(DeltaTime);
		UnacknowledgedMoves.Add(move);
		MovementComp->SimulateMove(move);
		Server_SendMove(move);
	}
	else if(GetOwner()->GetLocalRole() == ROLE_Authority && GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		// server and local control

		auto move = MovementComp->CreateMove(DeltaTime);
		Server_SendMove(move);
	}
	else if(GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		MovementComp->SimulateMove(ServerState.LastMove);
	}
}


void UGoKartMovementReplicator::Server_SendMove_Implementation(FGoKartMove Val)
{
	if(MovementComp == nullptr) return;
	
	MovementComp->SimulateMove(Val);

	ServerState.LastMove = Val;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComp->GetVelocity();
}

bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMove Val)
{
	// TODO: validate
	return true;
}


void UGoKartMovementReplicator::ClearAcknowledgedMoves(FGoKartMove LastMove)
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

void UGoKartMovementReplicator::OnRep_ServerState()
{
	GetOwner()->SetActorTransform(ServerState.Transform);
	
	if(MovementComp == nullptr) return;
	
	MovementComp->SetVelocity(ServerState.Velocity);

	ClearAcknowledgedMoves(ServerState.LastMove);
	for(const auto &move : UnacknowledgedMoves)
	{
		MovementComp->SimulateMove(move);
	}
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);
}
