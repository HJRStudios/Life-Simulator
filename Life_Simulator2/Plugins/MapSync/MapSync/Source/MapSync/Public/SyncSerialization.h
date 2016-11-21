
#pragma once

#include "GameFramework/Actor.h"
#include "SyncSerialization.generated.h"

UCLASS()
class ASyncSerialization : public AActor
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintNativeEvent)
	FString SerializeActor(AActor* ToSerialize);

	UFUNCTION(BlueprintNativeEvent)
	void DeserializeActor(const FString& Serialization, AActor* ToApply);

};
