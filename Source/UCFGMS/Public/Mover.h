// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mover.generated.h"

UCLASS()
class UCFGMS_API AMover : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMover();
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool mm=false;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
private:
	UPROPERTY(EditAnywhere)
	FVector moveoffset;
	UPROPERTY(EditAnywhere)
	float movetime=4;
	

	FVector origniallocation;
};
