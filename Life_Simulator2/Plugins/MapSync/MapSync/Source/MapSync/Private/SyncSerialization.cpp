
#include "MapSyncPrivatePCH.h"
#include "SyncSerialization.h"

ASyncSerialization::ASyncSerialization(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{

}

FString ASyncSerialization::SerializeActor_Implementation(AActor* ToSerialize)
{
	return "";
}

void ASyncSerialization::DeserializeActor_Implementation(const FString& Serialization, AActor* ToApply)
{
}
