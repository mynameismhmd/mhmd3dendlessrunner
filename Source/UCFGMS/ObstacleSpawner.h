#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ObstacleSpawner.generated.h"

USTRUCT(BlueprintType)
struct FObstacleSpawnInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    UStaticMesh* StaticMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    TSubclassOf<AActor> ObstacleActorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    float SpacingAfterindevisualObstacles = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    USkeletalMesh* SkeletalMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    FVector Scale = FVector(1.0f, 1.0f, 1.0f);

    // New property for specifying the location offset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    FVector LocationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    TSubclassOf<AActor> PlaneMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    float PlaneSpawnProbability = 0.5f;

    // New properties for the plane's transformation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    FVector PlaneLocationOffset = FVector(-500.0f, 0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    FRotator PlaneRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    FVector PlaneScale = FVector(1.0f, 1.0f, 1.0f);
    

};

USTRUCT(BlueprintType)
struct FObstacleSpawnParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    int32 NumObstacles = 5;
  

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    float SpacingBetweenObstacles = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    TArray<FObstacleSpawnInfo> ObstacleTypes;
};

UCLASS()
class UCFGMS_API AObstacleSpawner : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AObstacleSpawner();

    // Called every frame
    virtual void Tick(float DeltaTime) override;
    // Expose the Parameters property to the editor
     // Expose the Parameters property to the editor
    // Expose the SpawnParameters property to the editor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
    FObstacleSpawnParameters SpawnParameters;
    UFUNCTION(BlueprintCallable, Category = "Obstacles")
     
    TArray<AActor*> SpawnObstacles(const FObstacleSpawnParameters& Parameters,const TArray<FVector>& LanePositions);
   
   
protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
    
   
    };
