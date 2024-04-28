// Fill out your copyright notice in the Description page of Project Settings.


#include "Mover.h"
#include "math/UnrealMathUtility.h"
// Sets default values
AMover::AMover()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMover::BeginPlay()
{
	Super::BeginPlay();
	origniallocation=GetActorLocation();
}

// Called every frame
void AMover::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(mm)
	{
		FVector currentlocation=GetActorLocation();
		FVector targetlocation=origniallocation+moveoffset;
		float speed=FVector::Distance(origniallocation,targetlocation)/movetime;
		FVector newlocation=FMath::VInterpConstantTo(currentlocation,targetlocation,DeltaTime,speed);
		SetActorLocation(newlocation);
	}
}

