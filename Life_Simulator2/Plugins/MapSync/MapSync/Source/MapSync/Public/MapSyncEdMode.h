// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnrealEd.h" 
#include "Editor.h"
#include "Networking.h"

class FPackage;

class FMapSyncEdMode : public FEdMode
{
public:
	const static FEditorModeID EM_MapSyncEdModeId;
public:
	FMapSyncEdMode();
	virtual ~FMapSyncEdMode();

	virtual void Enter() override;
	virtual void Exit() override;
	bool UsesToolkits() const override;
	
private:

	void SendChange(const FString& ToSend);
	FString SerializeActor(AActor* Actor);
	FString GetStandardActorInfo(AActor* Actor);
	FString SerializeStaticMeshActor(AStaticMeshActor* Actor);
	FString SerializeBrushActor(ABrush* Actor);
	FString SerializeLightActor(ALight* Actor);

private:

	void DeserializeToActor(const FString& Serialization);
	void DeserializeToStaticMeshActor(const FString& Serialization, AStaticMeshActor* Actor);
	void DeserializeToBrushActor(const FString& Serialization, ABrush* Actor);
	void DeserializeToLightActor(const FString& Serialization, ALight* Actor);
	AActor* SpawnClassFromName(const FName& ClassName);

private:
	
	FSocket* ConnectionToServer;
	FIPv4Address ServerAdress;
	bool Connected;
	void SetServerAdress(const FString& ServerAdress);
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;

private:

	bool bActorInit;
	bool bInitialized;
	TArray<TPair<AActor*, FString>> LastActorMod;
	TArray<AActor*> LastSelectedActors;
	int32 InitialActorCount;
	void HandleActorsChange();
	void CheckActorChanged(AActor* Actor, const FString& Hash);
	void SendNewActor(AActor* NewActor);
	void SendDelActor(AActor* DelActor);
	void DeserializeNewActor(const FString& Serialization);
	void DeserializeDelActor(const FString& Serialization);

};
