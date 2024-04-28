#include "ObstacleSpawner.h"



// Sets default values
AObstacleSpawner::AObstacleSpawner()
{
    // Set this actor to call Tick() every frame.
    PrimaryActorTick.bCanEverTick = true;
   
}

// Called when the game starts or when spawned
void AObstacleSpawner::BeginPlay()
{
    Super::BeginPlay();
}

// Called every frame
void AObstacleSpawner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

TArray<AActor*> AObstacleSpawner::SpawnObstacles(const FObstacleSpawnParameters& Parameters, const TArray<FVector>& LanePositions)
{
    TArray<AActor*> SpawnedActors;
    TArray<int32> LaneSpawnCount;
    const int32 LaneCount = 3; // Since LanePositions is a fixed-size array of 3 elements
    LaneSpawnCount.Init(0, LaneCount);  // Initialize spawn counts for each lane
    float CurrentYPosition = LanePositions[0].Y; // Start Y position for spawning
    AActor* PreviouslySpawnedActor = nullptr;

    for (int32 ObstacleIndex = 0; ObstacleIndex < Parameters.NumObstacles; ++ObstacleIndex)
    {
        // Find lane with the least spawns
        int32 MinCount = LaneSpawnCount[0];
        for (int32 i = 1; i < LaneCount; ++i)
        {
            if (LaneSpawnCount[i] < MinCount)
                MinCount = LaneSpawnCount[i];
        }

        TArray<int32> LeastSpawnedLanes;
        for (int32 i = 0; i < LaneCount; ++i)
        {
            if (LaneSpawnCount[i] == MinCount)
                LeastSpawnedLanes.Add(i);
        }

        // Randomly choose a lane from those with the least spawns
        int32 LaneIndex = LeastSpawnedLanes[FMath::RandRange(0, LeastSpawnedLanes.Num() - 1)];
        LaneSpawnCount[LaneIndex]++;  // Increment spawn count for selected lane

        FVector SpawnPosition = FVector(LanePositions[LaneIndex].X, CurrentYPosition, LanePositions[LaneIndex].Z);

        // Calculate the next spawn position based on the previously spawned actor
        if (PreviouslySpawnedActor)
        {
            FVector PreviousLocation = PreviouslySpawnedActor->GetActorLocation();
            CurrentYPosition = PreviousLocation.Y + Parameters.SpacingBetweenObstacles;
            SpawnPosition.Y = CurrentYPosition;
        }

        int32 ObstacleTypeIndex = FMath::RandRange(0, Parameters.ObstacleTypes.Num() - 1);
        const FObstacleSpawnInfo& SpawnInfo = Parameters.ObstacleTypes[ObstacleTypeIndex];

        FVector ForwardVector;
        FVector BaseSpawnLocation;
        // Spawn either a static mesh or a skeletal mesh
        if (SpawnInfo.StaticMesh)
        {
            UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(this);
            MeshComponent->SetStaticMesh(SpawnInfo.StaticMesh);
            MeshComponent->SetWorldLocation(SpawnPosition + SpawnInfo.LocationOffset); // Apply the location offset
            MeshComponent->SetWorldRotation(SpawnInfo.Rotation);
            MeshComponent->SetWorldScale3D(SpawnInfo.Scale);
            MeshComponent->RegisterComponent();
            ForwardVector = MeshComponent->GetForwardVector();
            
           
            BaseSpawnLocation = MeshComponent->GetComponentLocation();
        }
     
        if (SpawnInfo.ObstacleActorClass)
        {
            FActorSpawnParameters SpawnParams;
           
            AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(SpawnInfo.ObstacleActorClass, SpawnPosition, SpawnInfo.Rotation, SpawnParams);
            SpawnedActor->SetActorScale3D(SpawnInfo.Scale);
            // Set the location and rotation if needed
            SpawnedActor->SetActorLocation(SpawnPosition + SpawnInfo.LocationOffset);
            SpawnedActor->SetActorRotation(SpawnInfo.Rotation);
           
          
            SpawnedActors.Add(SpawnedActor);
            ForwardVector = SpawnedActor->GetActorForwardVector();
            BaseSpawnLocation = SpawnedActor->GetActorLocation();
            PreviouslySpawnedActor = SpawnedActor;
        
        }
        else if (SpawnInfo.SkeletalMesh)
        {
            USkeletalMeshComponent* SkeletalComponent = NewObject<USkeletalMeshComponent>(this);
            SkeletalComponent->SetSkeletalMesh(SpawnInfo.SkeletalMesh);
            SkeletalComponent->SetWorldLocation(SpawnPosition + SpawnInfo.LocationOffset); // Apply the location offset
            SkeletalComponent->SetWorldRotation(SpawnInfo.Rotation);
            SkeletalComponent->SetWorldScale3D(SpawnInfo.Scale);
            SkeletalComponent->RegisterComponent();
            ForwardVector = SkeletalComponent->GetForwardVector();
            
           
            BaseSpawnLocation = SkeletalComponent->GetComponentLocation();
        }

        // Check if this is the train and if a plane should be spawned
        if (SpawnInfo.PlaneMesh)
        {
            float RandomChance = FMath::RandRange(0.0f, 1.0f);
            if (RandomChance <= SpawnInfo.PlaneSpawnProbability)
            {
                // Calculate position in front of the spawned actor for the plane
              
                float DistanceInFront = 0.0f; // Distance ahead to spawn the plane
                FVector PlaneSpawnPosition = BaseSpawnLocation + ForwardVector + SpawnInfo.PlaneLocationOffset;
                FActorSpawnParameters SpawnParams;
                
                // Spawn the plane actor
                AActor* NewPlaneActor = GetWorld()->SpawnActor<AActor>(SpawnInfo.PlaneMesh, PlaneSpawnPosition, SpawnInfo.PlaneRotation, SpawnParams);
                NewPlaneActor->SetActorScale3D(SpawnInfo.PlaneScale);
                
                NewPlaneActor->SetActorLocation(PlaneSpawnPosition);
                NewPlaneActor->SetActorRotation(SpawnInfo.PlaneRotation);

                SpawnedActors.Add(NewPlaneActor);
            }
        }
       

        UE_LOG(LogTemp, Warning, TEXT("Spawning Obstacle at Y Position: %f"), CurrentYPosition);
        CurrentYPosition += Parameters.SpacingBetweenObstacles+500.0f;
        UE_LOG(LogTemp, Warning, TEXT("Next Obstacle Y Position be: %f"), CurrentYPosition);
        
    }
    return SpawnedActors; 
}