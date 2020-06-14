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

	auto lastMove = MovementComp->GetLastMove();

	if(GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		UnacknowledgedMoves.Add(lastMove);
		Server_SendMove(lastMove);
	}
	else if(GetOwner()->GetLocalRole() == ROLE_Authority && GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		// server and local control

		UpdateServerState(lastMove);
	}
	else if(GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		SimulatedProxyTick(DeltaTime);
	}
}

void UGoKartMovementReplicator::SimulatedProxyTick(float DeltaTime)
{
	if(MovementComp == nullptr) return;
	
	SimulatedProxyTimeSinceLastUpdate += DeltaTime;

	if(SimulatedProxyTimeBetweenLastUpdates < KINDA_SMALL_NUMBER)
		return;

	float lerpRatio = SimulatedProxyTimeSinceLastUpdate / SimulatedProxyTimeBetweenLastUpdates;
	auto spline = CreateSpline();

	InterpolateLocation(spline, lerpRatio);
	InterpolateVelocity(spline, lerpRatio);
	InterpolateRotation(lerpRatio);
}

FHermiteCubicSpline UGoKartMovementReplicator::CreateSpline() const
{
	FHermiteCubicSpline spline;
	spline.StartLocation = SimulatedProxyStartTransform.GetLocation();
	spline.TargetLocation = ServerState.Transform.GetLocation();
	spline.StartDerivative = SimulatedProxyStartVelocity * SimulatedProxyTimeBetweenLastUpdates * 100;
	spline.TargetDerivative = ServerState.Velocity * SimulatedProxyTimeBetweenLastUpdates * 100;
	return spline;
}

void UGoKartMovementReplicator::InterpolateLocation(const FHermiteCubicSpline& Spline, float LerpRatio)
{
	if(MeshOffsetRoot == nullptr) return;
	
	auto newLocation = Spline.Interpolate(LerpRatio);
	MeshOffsetRoot->SetWorldLocation(newLocation);
}

void UGoKartMovementReplicator::InterpolateVelocity(const FHermiteCubicSpline& Spline, float LerpRatio)
{
	auto newDerivative = Spline.InterpolateDerivative(LerpRatio);
	auto newVelocity = newDerivative / (SimulatedProxyTimeBetweenLastUpdates * 100);
	MovementComp->SetVelocity(newVelocity);
}

void UGoKartMovementReplicator::InterpolateRotation(float LerpRatio)
{
	if(MeshOffsetRoot == nullptr) return;
	
	auto startRotation = SimulatedProxyStartTransform.GetRotation();
    auto targetRotation = ServerState.Transform.GetRotation();
    auto newRotation = FQuat::Slerp(startRotation, targetRotation, LerpRatio);
	MeshOffsetRoot->SetWorldRotation(newRotation);
}

void UGoKartMovementReplicator::Server_SendMove_Implementation(FGoKartMove Val)
{
	if(MovementComp == nullptr) return;

	ClientSimulatedTime += Val.DeltaTime;
	
	MovementComp->SimulateMove(Val);

	UpdateServerState(Val);
	
}

bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMove Val)
{
	auto proposedTime = ClientSimulatedTime + Val.DeltaTime;
	bool clientNotRunningAhead = proposedTime < GetWorld()->TimeSeconds;
	if(!clientNotRunningAhead)
	{
		UE_LOG(LogTemp, Error, TEXT("Client is running too fast."))
		return false;
	}
	
	if(!Val.IsValidInput())
	{
		UE_LOG(LogTemp, Error, TEXT("Client has invalid input."))
		return false;
	}

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
	switch (GetOwnerRole())
	{
	case ROLE_SimulatedProxy:
		SimulatedProxy_OnRep_ServerState();
		break;
	case ROLE_AutonomousProxy:
		AutonomousProxy_OnRep_ServerState();
		break;
	default:
		break;
	}
}

void UGoKartMovementReplicator::AutonomousProxy_OnRep_ServerState()
{
	GetOwner()->SetActorTransform(ServerState.Transform);

	if (MovementComp == nullptr) return;

	MovementComp->SetVelocity(ServerState.Velocity);

	ClearAcknowledgedMoves(ServerState.LastMove);
	for (const auto& move : UnacknowledgedMoves)
	{
		MovementComp->SimulateMove(move);
	}
}

void UGoKartMovementReplicator::SimulatedProxy_OnRep_ServerState()
{
	if(MovementComp == nullptr) return;
	
	SimulatedProxyTimeBetweenLastUpdates = SimulatedProxyTimeSinceLastUpdate;
	SimulatedProxyTimeSinceLastUpdate = 0;

	if(MeshOffsetRoot)
	{
		SimulatedProxyStartTransform = MeshOffsetRoot->GetComponentTransform(); 
	}
	
	SimulatedProxyStartVelocity = MovementComp->GetVelocity();

	GetOwner()->SetActorTransform(ServerState.Transform);
}

void UGoKartMovementReplicator::UpdateServerState(const FGoKartMove &Val)
{
	ServerState.LastMove = Val;
    ServerState.Transform = GetOwner()->GetActorTransform();
    ServerState.Velocity = MovementComp->GetVelocity();
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);
}
