// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MapSyncPrivatePCH.h"
#include "MapSyncEdMode.h"
#include "MapSyncEdModeToolkit.h"
#include "Toolkits/ToolkitManager.h"
#include "Engine/StaticMeshActor.h"
#include "ScopedTransaction.h"
#include "SyncSerialization.h"
#include <string>

const FEditorModeID FMapSyncEdMode::EM_MapSyncEdModeId = TEXT("EM_MapSync");

FMapSyncEdMode::FMapSyncEdMode()
{
	Connected = false;
	bActorInit = false;
	bInitialized = false;
}

FMapSyncEdMode::~FMapSyncEdMode()
{
}

void FMapSyncEdMode::Enter()
{
	FEdMode::Enter();

	GetWorld()->Exec(GetWorld(), TEXT("Log LogDerivedDataCache Log"));
	GetWorld()->Exec(GetWorld(), TEXT("Log LogWindowsDesktop Log"));
	GetWorld()->Exec(GetWorld(), TEXT("Log LogTextStoreACP Log"));

	GConfig->RemoveKey(TEXT("Core.Log"), TEXT("ForceLogLinker"), GEngineIni);
	FString IniForceLog;
	if (!GConfig->GetString(TEXT("MapSync"), TEXT("Server"), IniForceLog, GEditorIni) || IniForceLog != "true")
	{
		GetWorld()->Exec(GetWorld(), TEXT("log LogLinker all off"));
		GConfig->SetString(TEXT("Core.Log"), TEXT("LogLinker"), TEXT("all off"), GEngineIni);
		GConfig->SetString(TEXT("Core.Log"), TEXT("LogNet"), TEXT("verbose off"), GEngineIni);
	}

	GConfig->Flush(false, GEngineIni);

	if(!Connected)
	{
		FString IniIp;
		if(GConfig->GetString(TEXT("MapSync"), TEXT("Server"), IniIp, GEditorIni))
		{
			SetServerAdress(IniIp);
		}

		/*
		// TODO: utiliser ça plutôt que les propriétés au cas par cas
		FCoreUObjectDelegates::OnObjectPropertyChanged.AddLambda([](UObject* Obj, FPropertyChangedEvent& ChangedProp)
		{
			// UClass::FindPropertyByName(FName);
			// ChangedProp.Property->GetFName();
			// UProperty::ContainerPtrToValuePtr<type>(Obj)
			// La classe étant donnée par l'objet en lui même
			// En faisant une carte des principaux types de données, ça se fait
		});
		*/
	}

	if (!bActorInit)
	{
		// Fill LastActorMod, because this code is executed at first module execution
		for (TActorIterator<AActor> Itr(GetWorld()); Itr; ++Itr)
		{
			if (Itr->IsA(ULevel::StaticClass()) || Itr->IsA(ALevelScriptActor::StaticClass()))
			{
				continue;
			}

			FString Serialized = SerializeActor(*Itr);
			LastActorMod.Add(TPairInitializer<AActor*, FString>(*Itr, Serialized.Left(Serialized.Len()-1)));
		}

		InitialActorCount = GetWorld()->GetActorCount();
		bActorInit = true;
	}

	bInitialized = true;

	if (!Toolkit.IsValid() && UsesToolkits())
	{
		Toolkit = MakeShareable(new FMapSyncEdModeToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
	}
}

void FMapSyncEdMode::Exit()
{
	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}
	
	bInitialized = false;

	// Call base Exit method to ensure proper cleanup
	FEdMode::Exit();
}

bool FMapSyncEdMode::UsesToolkits() const
{
	return true;
}

FString FMapSyncEdMode::SerializeActor(AActor* Object)
{
	if (!Object)
	{
		return "nullptr";
	}

	FString ToReturn = Object->GetFName().ToString();
	
	AActor* Actor = Cast<AActor>(Object);
	if (Actor->IsA(AStaticMeshActor::StaticClass()) && Cast<AStaticMeshActor>(Actor)->GetStaticMeshComponent())
	{
		ToReturn += " staticmeshactor " + GetStandardActorInfo(Actor) + " ";
		ToReturn += SerializeStaticMeshActor(Cast<AStaticMeshActor>(Actor));
	}
	else if (Actor->IsA(ABrush::StaticClass()) && Cast<ABrush>(Actor)->BrushBuilder)
	{
		ToReturn += " brush " + GetStandardActorInfo(Actor) + " ";
		ToReturn += SerializeBrushActor(Cast<ABrush>(Actor));
	}
	else if (Actor->IsA(ALight::StaticClass()))
	{
		ToReturn += " light " + GetStandardActorInfo(Actor) + " ";
		ToReturn += SerializeLightActor(Cast<ALight>(Actor));
	}
	else
	{
		ToReturn += " actor " + GetStandardActorInfo(Actor);
	}

	ToReturn += "#";

	FString ClassName = Actor->GetClass()->GetFName().ToString();
	if (ClassName.Right(2) == "_C")
	{
		ClassName = ClassName.Left(ClassName.Len()-2);
	}

	UClass* ClassToLook = Actor->GetClass();
	while (ClassToLook)
	{
		FString ClassToLookNameName = ClassToLook->GetFName().ToString();
		if (ClassToLookNameName.Right(2) == "_C")
		{
			ClassToLookNameName = ClassToLookNameName.Left(ClassToLookNameName.Len() - 2);
		}

		// Will log if not found... Not a good thing, could be corrected in future releases ?
		UBlueprint* BPClass = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *("/Game/MapSync/" + ClassToLookNameName + "Sync"), nullptr, LOAD_NoWarn));
		if (BPClass)
		{
			ASyncSerialization* FunctionHandler = Cast<ASyncSerialization>(BPClass->GeneratedClass->GetDefaultObject());
			if (FunctionHandler)
			{
				ToReturn += FunctionHandler->SerializeActor(Actor);
				break;
			}
		}
		ClassToLook = ClassToLook->GetSuperClass();
	}
	
	return ToReturn + ";";
}

FString FMapSyncEdMode::GetStandardActorInfo(AActor* Actor)
{
	FString ToReturn = "";
	FVector Loc = Actor->GetActorLocation();
	ToReturn +=		  FString::SanitizeFloat(Loc.X);
	ToReturn += " " + FString::SanitizeFloat(Loc.Y);
	ToReturn += " " + FString::SanitizeFloat(Loc.Z);
	FRotator Rot = Actor->GetActorRotation();
	ToReturn += " " + FString::SanitizeFloat(Rot.Pitch); // Order to follow FRotator's ctor order
	ToReturn += " " + FString::SanitizeFloat(Rot.Yaw);
	ToReturn += " " + FString::SanitizeFloat(Rot.Roll);
	FVector Scale = Actor->GetActorScale();
	ToReturn += " " + FString::SanitizeFloat(Scale.X);
	ToReturn += " " + FString::SanitizeFloat(Scale.Y);
	ToReturn += " " + FString::SanitizeFloat(Scale.Z);
	return ToReturn;
}

FString FMapSyncEdMode::SerializeStaticMeshActor(AStaticMeshActor* Actor)
{
	FString ToReturn = "";
	ToReturn += FStringAssetReference(Actor->GetStaticMeshComponent()->StaticMesh).ToString();
	ToReturn += " " + FString::FromInt(Actor->GetStaticMeshComponent()->GetNumMaterials());
	TArray<UMaterialInterface*> Materials = Actor->GetStaticMeshComponent()->GetMaterials();
	for (auto& it : Materials)
	{
		if(it)
		{
			ToReturn += " " + FStringAssetReference(it->GetMaterial()).ToString();
		}
	}
	return ToReturn;
}

FString FMapSyncEdMode::SerializeBrushActor(ABrush* Actor)
{
	if (!Actor || !Actor->BrushBuilder)
	{
		return "";
	}

	FString ToReturn = "";
	if (Actor->BrushType == EBrushType::Brush_Add)
	{
		ToReturn += "add";
	}
	else
	{
		ToReturn += "sub";
	}

	if (Actor->BrushBuilder->IsA(UCubeBuilder::StaticClass()))
	{
		ToReturn += " cube";
		UCubeBuilder* CubeBuilder = Cast<UCubeBuilder>(Actor->BrushBuilder);
		ToReturn += " " + FString::SanitizeFloat(CubeBuilder->X);
		ToReturn += " " + FString::SanitizeFloat(CubeBuilder->Y);
		ToReturn += " " + FString::SanitizeFloat(CubeBuilder->Z);
	}
	else if (Actor->BrushBuilder->IsA(UConeBuilder::StaticClass()))
	{
		ToReturn += " cone";
		UConeBuilder* ConeBuilder = Cast<UConeBuilder>(Actor->BrushBuilder);
		ToReturn += " " + FString::SanitizeFloat(ConeBuilder->Z);
		ToReturn += " " + FString::SanitizeFloat(ConeBuilder->OuterRadius);
		ToReturn += " " + FString::FromInt(ConeBuilder->Sides);
	}
	else if (Actor->BrushBuilder->IsA(UCurvedStairBuilder::StaticClass()))
	{
		ToReturn += " curvedstair";
		UCurvedStairBuilder* CurvedStairsBuilder = Cast<UCurvedStairBuilder>(Actor->BrushBuilder);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->InnerRadius);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->StepHeight);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->StepWidth);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->AngleOfCurve);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->NumSteps);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->AddToFirstStep);
	}
	else if (Actor->BrushBuilder->IsA(UCylinderBuilder::StaticClass()))
	{
		ToReturn += " cylinder";
		UCylinderBuilder* CylinderBuilder = Cast<UCylinderBuilder>(Actor->BrushBuilder);
		ToReturn += " " + FString::SanitizeFloat(CylinderBuilder->Z);
		ToReturn += " " + FString::SanitizeFloat(CylinderBuilder->OuterRadius);
		ToReturn += " " + FString::FromInt(CylinderBuilder->Sides);
	}
	else if (Actor->BrushBuilder->IsA(ULinearStairBuilder::StaticClass()))
	{
		ToReturn += " linearstair";
		ULinearStairBuilder* CurvedStairsBuilder = Cast<ULinearStairBuilder>(Actor->BrushBuilder);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->StepLength);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->StepHeight);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->StepWidth);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->NumSteps);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->AddToFirstStep);
	}
	else if (Actor->BrushBuilder->IsA(USpiralStairBuilder::StaticClass()))
	{
		ToReturn += " spiralstair";
		USpiralStairBuilder* CurvedStairsBuilder = Cast<USpiralStairBuilder>(Actor->BrushBuilder);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->InnerRadius);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->StepWidth);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->StepHeight);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->StepThickness);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->NumStepsPer360);
		ToReturn += " " + FString::FromInt(CurvedStairsBuilder->NumSteps);
	}
	else if (Actor->BrushBuilder->IsA(UTetrahedronBuilder::StaticClass()))
	{
		ToReturn += " sphere";
		UTetrahedronBuilder* SphereBuilder = Cast<UTetrahedronBuilder>(Actor->BrushBuilder);
		ToReturn += " " + FString::SanitizeFloat(SphereBuilder->Radius);
		ToReturn += " " + FString::FromInt(SphereBuilder->SphereExtrapolation);
	}

	return ToReturn;
}

FString FMapSyncEdMode::SerializeLightActor(ALight* Actor)
{
	FString ToReturn = "";
	ToReturn += FString::SanitizeFloat(Actor->GetLightComponent()->Intensity);
	ToReturn += " " + FString::FromInt(Actor->GetLightComponent()->LightColor.R);
	ToReturn += " " + FString::FromInt(Actor->GetLightComponent()->LightColor.G);
	ToReturn += " " + FString::FromInt(Actor->GetLightComponent()->LightColor.B);
	return "";
}

void FMapSyncEdMode::DeserializeToActor(const FString& Serialization)
{
	TArray<FString> BigParts;
	Serialization.ParseIntoArray(BigParts, TEXT("#"));
	if (BigParts.Num() < 1)
	{
		return;
	}

	TArray<FString> Parts;
	BigParts[0].ParseIntoArray(Parts, TEXT(" "));
	if (Parts.Num() < 10)
	{
		return;
	}

	FName Name = FName(*Parts[0]);
	FString ActorType = Parts[1];
	for (TActorIterator<AActor> Itr(GetWorld()); Itr; ++Itr)
	{
		if (Itr->GetFName() == Name)
		{
			Itr->SetActorLocation(FVector(	FCString::Atof(*Parts[2]), FCString::Atof(*Parts[3]), FCString::Atof(*Parts[4])));
			Itr->SetActorRotation(FRotator(	FCString::Atof(*Parts[5]), FCString::Atof(*Parts[6]), FCString::Atof(*Parts[7])));
			Itr->SetActorScale3D(FVector(	FCString::Atof(*Parts[8]), FCString::Atof(*Parts[9]), FCString::Atof(*Parts[10])));
			int32 Index = Parts[0].Len() + Parts[1].Len() + Parts[2].Len() + Parts[3].Len() + Parts[4].Len() + Parts[5].Len() + Parts[6].Len() + Parts[7].Len() + Parts[8].Len() + Parts[9].Len() + Parts[10].Len() + 11;
			if (ActorType == "staticmeshactor" && Itr->IsA(AStaticMeshActor::StaticClass()))
			{
				DeserializeToStaticMeshActor(BigParts[0].Right(BigParts[0].Len() - Index ), Cast<AStaticMeshActor>(*Itr));
			}
			else if (ActorType == "brush" && Itr->IsA(ABrush::StaticClass()))
			{
				DeserializeToBrushActor(BigParts[0].Right(BigParts[0].Len() - Index), Cast<ABrush>(*Itr));
			}
			// else if (ActorType == "light" && Itr->IsA(ALight::StaticClass()))
			// {
				// DeserializeToLightActor(BigParts[0].Right(BigParts[0].Len() - Index), Cast<ALight>(*Itr));
			// }

			if (BigParts.Num() < 2)
			{
				return;
			}
			
			UClass* ClassToLook = (*Itr)->GetClass();
			while(ClassToLook)
			{
				FString ClassName = ClassToLook->GetFName().ToString();
				if (ClassName.Right(2) == "_C")
				{
					ClassName = ClassName.Left(ClassName.Len() - 2);
				}

				// Will log if not found... Not a good thing, could be corrected in future releases ?
				UBlueprint* BPClass = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *("/Game/MapSync/" + ClassName + "Sync")));// nullptr, LOAD_NoWarn));
				if (BPClass)
				{
					ASyncSerialization* FunctionHandler = Cast<ASyncSerialization>(BPClass->GeneratedClass->GetDefaultObject());
					if (FunctionHandler)
					{
						FunctionHandler->DeserializeActor(BigParts[1], *Itr);
						break;
					}
				}
				ClassToLook = ClassToLook->GetSuperClass();
			}

			continue;
		}
	}
}

void FMapSyncEdMode::DeserializeToStaticMeshActor(const FString& Serialization, AStaticMeshActor* Actor)
{
	TArray<FString> Parts;
	Serialization.ParseIntoArray(Parts, TEXT(" "));
	if (Parts.Num() >= 2) // The first is the asset reference, the second is the materials count
	{
		// Set mesh
		if (FStringAssetReference(Actor->GetStaticMeshComponent()->StaticMesh).ToString() != Parts[0])
		{
			GetWorld()->Exec(GetWorld(), TEXT("Log LogLinker off"));
			UStaticMesh* FoundMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), Actor, *Parts[0]));
			if (FoundMesh)
			{
				EComponentMobility::Type OldMobility = Actor->GetStaticMeshComponent()->Mobility;
				Actor->SetMobility(EComponentMobility::Movable);
				Actor->GetStaticMeshComponent()->SetStaticMesh(FoundMesh);
				Actor->SetMobility(OldMobility);
			}
		}

		// Set materials
		int32 MaterialsCount = FCString::Atoi(*Parts[1]);
		if (Parts.Num() >= MaterialsCount + 2)
		{
			// For each received material
			for(int32 i = 0; i < FMath::Min(MaterialsCount, Actor->GetStaticMeshComponent()->GetNumMaterials()); i++)
			{
				if (FStringAssetReference(Actor->GetStaticMeshComponent()->GetMaterial(i)->GetMaterial()) != Parts[i+2])
				{
					GetWorld()->Exec(GetWorld(), TEXT("Log LogLinker off"));
					UMaterial* FoundMat = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), Actor, *Parts[i + 2]));
					if (FoundMat)
					{
						Actor->GetStaticMeshComponent()->SetMaterial(i, FoundMat);
					}
				}
			}
		}
	}
}

void FMapSyncEdMode::DeserializeToBrushActor(const FString& Serialization, ABrush* Actor)
{
	if (!Actor || !Actor->GetBrushBuilder())
	{
		return;
	}

	// If no change, do nothing
	if (SerializeBrushActor(Actor) == Serialization)
	{
		return;
	}

	TArray<FString> Parts;
	Serialization.ParseIntoArray(Parts, TEXT(" "));
	if (Parts.Num() >= 2)
	{
		// Add or substract
		if (Parts[0] == "add" && Actor->BrushType != EBrushType::Brush_Add)
		{
			Actor->BrushType == EBrushType::Brush_Add;
		}
		else if (Parts[0] == "sub" && Actor->BrushType != EBrushType::Brush_Subtract)
		{
			Actor->BrushType == EBrushType::Brush_Subtract;
		}

		// Rebuild BSP
		if (Parts[1] == "cube" && Parts.Num() - 2 >= 3)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "BrushSet", "Brush Set"));

			UCubeBuilder* CubeBuilder = NewObject<UCubeBuilder>(Actor, UCubeBuilder::StaticClass(), NAME_None, RF_Transactional);
			CubeBuilder->X = FCString::Atof(*Parts[2]);
			CubeBuilder->Y = FCString::Atof(*Parts[3]);
			CubeBuilder->Z = FCString::Atof(*Parts[4]);
			
			CubeBuilder->Build(GetWorld(), Actor);
			GEditor->RebuildAlteredBSP();

			if (Actor->BrushBuilder->IsA(UCubeBuilder::StaticClass()))
			{
				Cast<UCubeBuilder>(Actor->BrushBuilder)->X = FCString::Atof(*Parts[2]);
				Cast<UCubeBuilder>(Actor->BrushBuilder)->Y = FCString::Atof(*Parts[3]);
				Cast<UCubeBuilder>(Actor->BrushBuilder)->Z = FCString::Atof(*Parts[4]);
			}
		}
		else if (Parts[1] == "cone" && Parts.Num() - 2 >= 3)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "BrushSet", "Brush Set"));

			UConeBuilder* ConeBuilder = NewObject<UConeBuilder>(Actor, UConeBuilder::StaticClass(), NAME_None, RF_Transactional);
			ConeBuilder->Z = FCString::Atof(*Parts[2]);
			ConeBuilder->OuterRadius = FCString::Atof(*Parts[3]);
			ConeBuilder->Sides = FCString::Atoi(*Parts[4]);

			ConeBuilder->Build(GetWorld(), Actor);
			GEditor->RebuildAlteredBSP();

			if (Actor->BrushBuilder->IsA(UConeBuilder::StaticClass()))
			{
				Cast<UConeBuilder>(Actor->BrushBuilder)->Z = FCString::Atof(*Parts[2]);
				Cast<UConeBuilder>(Actor->BrushBuilder)->OuterRadius = FCString::Atof(*Parts[3]);
				Cast<UConeBuilder>(Actor->BrushBuilder)->Sides = FCString::Atoi(*Parts[4]);
			}
		}
		else if (Parts[1] == "curvedstair" && Parts.Num() - 2 >= 6)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "BrushSet", "Brush Set"));

			UCurvedStairBuilder* CurvedStairBuilder = NewObject<UCurvedStairBuilder>(Actor, UCurvedStairBuilder::StaticClass(), NAME_None, RF_Transactional);
			CurvedStairBuilder->InnerRadius = FCString::Atoi(*Parts[2]);
			CurvedStairBuilder->StepHeight = FCString::Atoi(*Parts[3]);
			CurvedStairBuilder->StepWidth = FCString::Atoi(*Parts[4]);
			CurvedStairBuilder->AngleOfCurve = FCString::Atoi(*Parts[5]);
			CurvedStairBuilder->NumSteps = FCString::Atoi(*Parts[6]);
			CurvedStairBuilder->AddToFirstStep = FCString::Atoi(*Parts[7]);

			CurvedStairBuilder->Build(GetWorld(), Actor);
			GEditor->RebuildAlteredBSP();

			if (Actor->BrushBuilder->IsA(UCurvedStairBuilder::StaticClass()))
			{
				Cast<UCurvedStairBuilder>(Actor->BrushBuilder)->InnerRadius = FCString::Atoi(*Parts[2]);
				Cast<UCurvedStairBuilder>(Actor->BrushBuilder)->StepHeight = FCString::Atoi(*Parts[3]);
				Cast<UCurvedStairBuilder>(Actor->BrushBuilder)->StepWidth = FCString::Atoi(*Parts[4]);
				Cast<UCurvedStairBuilder>(Actor->BrushBuilder)->AngleOfCurve = FCString::Atoi(*Parts[5]);
				Cast<UCurvedStairBuilder>(Actor->BrushBuilder)->NumSteps = FCString::Atoi(*Parts[6]);
				Cast<UCurvedStairBuilder>(Actor->BrushBuilder)->AddToFirstStep = FCString::Atoi(*Parts[7]);
			}
		}
		else if (Parts[1] == "cylinder" && Parts.Num() - 2 >= 3)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "BrushSet", "Brush Set"));

			UCylinderBuilder* CylinderBuilder = NewObject<UCylinderBuilder>(Actor, UCylinderBuilder::StaticClass(), NAME_None, RF_Transactional);
			CylinderBuilder->Z = FCString::Atof(*Parts[2]);
			CylinderBuilder->OuterRadius = FCString::Atof(*Parts[3]);
			CylinderBuilder->Sides = FCString::Atoi(*Parts[4]);

			CylinderBuilder->Build(GetWorld(), Actor);
			GEditor->RebuildAlteredBSP();
			
			if(Actor->BrushBuilder->IsA(UConeBuilder::StaticClass()))
			{
				Cast<UConeBuilder>(Actor->BrushBuilder)->Z = FCString::Atof(*Parts[2]);
				Cast<UConeBuilder>(Actor->BrushBuilder)->OuterRadius = FCString::Atof(*Parts[3]);
				Cast<UConeBuilder>(Actor->BrushBuilder)->Sides = FCString::Atoi(*Parts[4]);
			}
		}
		else if (Parts[1] == "linearstair" && Parts.Num() - 2 >= 5)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "BrushSet", "Brush Set"));

			ULinearStairBuilder* LinearStairBuilder = NewObject<ULinearStairBuilder>(Actor, ULinearStairBuilder::StaticClass(), NAME_None, RF_Transactional);
			LinearStairBuilder->StepLength = FCString::Atoi(*Parts[2]);
			LinearStairBuilder->StepHeight = FCString::Atoi(*Parts[3]);
			LinearStairBuilder->StepWidth = FCString::Atoi(*Parts[4]);
			LinearStairBuilder->NumSteps = FCString::Atoi(*Parts[5]);
			LinearStairBuilder->AddToFirstStep = FCString::Atoi(*Parts[6]);

			LinearStairBuilder->Build(GetWorld(), Actor);
			GEditor->RebuildAlteredBSP();

			if (Actor->BrushBuilder->IsA(ULinearStairBuilder::StaticClass()))
			{
				Cast<ULinearStairBuilder>(Actor->BrushBuilder)->StepLength = FCString::Atoi(*Parts[2]);
				Cast<ULinearStairBuilder>(Actor->BrushBuilder)->StepHeight = FCString::Atoi(*Parts[3]);
				Cast<ULinearStairBuilder>(Actor->BrushBuilder)->StepWidth = FCString::Atoi(*Parts[4]);
				Cast<ULinearStairBuilder>(Actor->BrushBuilder)->NumSteps = FCString::Atoi(*Parts[5]);
				Cast<ULinearStairBuilder>(Actor->BrushBuilder)->AddToFirstStep = FCString::Atoi(*Parts[6]);
			}
		}
		else if (Parts[1] == "spiralstair" && Parts.Num() - 2 >= 6)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "BrushSet", "Brush Set"));

			USpiralStairBuilder* SpiralStairBuilder = NewObject<USpiralStairBuilder>(Actor, USpiralStairBuilder::StaticClass(), NAME_None, RF_Transactional);
			SpiralStairBuilder->InnerRadius = FCString::Atoi(*Parts[2]);
			SpiralStairBuilder->StepWidth = FCString::Atoi(*Parts[3]);
			SpiralStairBuilder->StepHeight = FCString::Atoi(*Parts[4]);
			SpiralStairBuilder->StepThickness = FCString::Atoi(*Parts[5]);
			SpiralStairBuilder->NumStepsPer360 = FCString::Atoi(*Parts[6]);
			SpiralStairBuilder->NumSteps = FCString::Atoi(*Parts[7]);

			SpiralStairBuilder->Build(GetWorld(), Actor);
			GEditor->RebuildAlteredBSP();

			if (Actor->BrushBuilder->IsA(USpiralStairBuilder::StaticClass()))
			{
				Cast<USpiralStairBuilder>(Actor->BrushBuilder)->InnerRadius = FCString::Atoi(*Parts[2]);
				Cast<USpiralStairBuilder>(Actor->BrushBuilder)->StepWidth = FCString::Atoi(*Parts[3]);
				Cast<USpiralStairBuilder>(Actor->BrushBuilder)->StepHeight = FCString::Atoi(*Parts[4]);
				Cast<USpiralStairBuilder>(Actor->BrushBuilder)->StepThickness = FCString::Atoi(*Parts[5]);
				Cast<USpiralStairBuilder>(Actor->BrushBuilder)->NumStepsPer360 = FCString::Atoi(*Parts[6]);
				Cast<USpiralStairBuilder>(Actor->BrushBuilder)->NumSteps = FCString::Atoi(*Parts[7]);
			}
		}
		else if (Parts[1] == "sphere" && Parts.Num() - 2 >= 2)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "BrushSet", "Brush Set"));

			UTetrahedronBuilder* SphereBuilder = NewObject<UTetrahedronBuilder>(Actor, UTetrahedronBuilder::StaticClass(), NAME_None, RF_Transactional);
			SphereBuilder->Radius = FCString::Atof(*Parts[2]);
			SphereBuilder->SphereExtrapolation = FCString::Atoi(*Parts[3]);

			SphereBuilder->Build(GetWorld(), Actor);
			GEditor->RebuildAlteredBSP();

			if (Actor->BrushBuilder->IsA(UTetrahedronBuilder::StaticClass()))
			{
				Cast<UTetrahedronBuilder>(Actor->BrushBuilder)->Radius = FCString::Atof(*Parts[2]);
				Cast<UTetrahedronBuilder>(Actor->BrushBuilder)->SphereExtrapolation = FCString::Atoi(*Parts[3]);
			}
		}
	}
}

void FMapSyncEdMode::DeserializeToLightActor(const FString& Serialization, ALight* Actor)
{
	// TODO
}

void FMapSyncEdMode::SetServerAdress(const FString& StringAdress)
{
	TArray<FString> IPPortStrs;
	StringAdress.ParseIntoArray(IPPortStrs, TEXT(":"));
	if (IPPortStrs.Num() < 2)
	{
		return;
	}

	FIPv4Address::Parse(*IPPortStrs[0], ServerAdress);
	ConnectionToServer = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);
	
	TSharedRef<FInternetAddr> InetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	// ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetHostByName("evols-evols-1.c9.io", *InetAddr);
	
	InetAddr->SetIp(ServerAdress.Value);
	InetAddr->SetPort(FCString::Atoi(*IPPortStrs[1]));
	Connected = ConnectionToServer->Connect(*InetAddr);
	if(!Connected)
	{
		UE_LOG(LogMapSync, Warning, TEXT("======================="));
		UE_LOG(LogMapSync, Warning, TEXT("MapSync was unable to connect to server !"));
		UE_LOG(LogMapSync, Warning, TEXT("Server ip: %s, port: %d"), *IPPortStrs[0], FCString::Atoi(*IPPortStrs[1]));
		UE_LOG(LogMapSync, Warning, TEXT("======================="));
		return;
	}
	else
	{
		UE_LOG(LogMapSync, Log, TEXT("MapSync successfully connected to server !"));
	}
}

AActor* FMapSyncEdMode::SpawnClassFromName(const FName& ClassName)
{
	UClass* Class = nullptr;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if ((*It) && (*It)->GetFName() == ClassName)
		{
			Class = *It;
			break;
		}
	}
	if (!Class)
	{
		return nullptr;
	}

	return GetWorld()->SpawnActor<AActor>(Class);
}

void FMapSyncEdMode::SendChange(const FString& ToSend)
{
	FString FormatedStr = GetWorld()->GetFName().ToString() + " update " + ToSend;
	const TCHAR* SerializedChar = FormatedStr.GetCharArray().GetData();
	int32 Size = FCString::Strlen(SerializedChar);
	int32 Sent = 0;
	ConnectionToServer->Send((uint8*)(TCHAR_TO_UTF8(SerializedChar)), Size, Sent);
}

void FMapSyncEdMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	FEdMode::Tick(ViewportClient, DeltaTime);
	if(!ViewportClient)
	{
		return;
	}
	
	if(bActorInit) // Don't do it if unitialized
	{
		HandleActorsChange();
	}

	uint32 ReceivedSize = 0;
	if (Connected && ConnectionToServer && ConnectionToServer->HasPendingData(ReceivedSize))
	{
		uint8* Data = new uint8[ReceivedSize];
		int32 DataRead = 0;
		ConnectionToServer->Recv(Data, ReceivedSize, DataRead, ESocketReceiveFlags::None);
		const std::string ReceivedAsCppStr(reinterpret_cast<const char*>(Data), ReceivedSize); // <- this idea is from Rama (see https://wiki.unrealengine.com/TCP_Socket_Listener,_Receive_Binary_Data_From_an_IP/Port_Into_UE4,_(Full_Code_Sample)#Core_Research_I_Am_Sharing_with_You)
		FString ReceivedStr(ReceivedAsCppStr.c_str());

		TArray<FString> Requests;
		ReceivedStr.ParseIntoArray(Requests, TEXT(";"));
		for (auto& it : Requests)
		{
			TArray<FString> ReqParts;
			it.ParseIntoArray(ReqParts, TEXT(" "));
			if(ReqParts.Num() >= 2 && ReqParts[0] == GetWorld()->GetFName().ToString())
			{
				if (ReqParts[1] == "update")
				{
					DeserializeToActor(it.Right(it.Len() - (ReqParts[0].Len() + ReqParts[1].Len() + 2)));
				}
				else if (ReqParts[1] == "create")
				{
					DeserializeNewActor(it.Right(it.Len() - (ReqParts[0].Len() + ReqParts[1].Len() + 2)));
				}
				else if (ReqParts[1] == "delete")
				{
					DeserializeDelActor(it.Right(it.Len() - (ReqParts[0].Len() + ReqParts[1].Len() + 2)));
				}
			}
		}
	}
}

void FMapSyncEdMode::HandleActorsChange()
{
	// TODO: Quand un actor est en pendingkill et qu'il est ctrl+z, il revient
	// Je sais pas comment le faire revenir avec ses components
	// Il y a cette fonction qui pourrait être pas mal quand même:
	// GEngine->BroadcastLevelActorAdded()

	TArray<AActor*> Local_LastSelectedActors;
	for (FSelectionIterator SelectionIt = GEditor->GetSelectedActorIterator(); SelectionIt; ++SelectionIt)
	{
		AActor* SelectedActor = Cast<AActor>(*SelectionIt);
		if(!SelectedActor)
		{
			continue;
		}
		
		Local_LastSelectedActors.AddUnique(SelectedActor);
		for (int32 LAMIndex = 0; LAMIndex < LastActorMod.Num(); LAMIndex++)
		{
			if (LastActorMod[LAMIndex].Key == *SelectionIt)
			{
				FString ToSend = SerializeActor(SelectedActor);
				if(ToSend != LastActorMod[LAMIndex].Value)
				{
					LastActorMod[LAMIndex].Value = ToSend;
					SendChange(ToSend);
				}
			}

			bool bFound = false;
			for (auto& InListActor : LastActorMod)
			{
				if (SelectedActor->IsA(ALevelScriptActor::StaticClass()))
				{
					continue;
				}
				if (InListActor.Key == SelectedActor)
				{
					bFound = true;
					break;
				}
			}
			if (!bFound)
			{
				if (SelectedActor->IsA<ABrush>())
				{
					UE_LOG(LogMapSync, Log, TEXT("MapSync: Sending new BSP Brush, this is NOT a fully supported feature, it may cause bugs"));
				}

				FString Serialization = SerializeActor(SelectedActor);
				SendNewActor(SelectedActor);
				SendChange(Serialization);
				LastActorMod.Add(TPairInitializer<AActor*, FString>(SelectedActor, Serialization));
			}
		}
	}

	if (Local_LastSelectedActors != LastSelectedActors)// && Local_LastSelectedActors.IsValidIndex(0))
	{
		for (auto& ActorToDelete : LastSelectedActors)
		{
			if (ActorToDelete->IsPendingKill())
			{
				for (auto& ActorMod : LastActorMod)
				{
					if (ActorMod.Key == ActorToDelete && ActorMod.Value != "removed")
					{
						SendDelActor(ActorToDelete);
						ActorMod.Value = "removed";
						break;
					}
				}
			}
		}
		LastSelectedActors = Local_LastSelectedActors;
	}
}

void FMapSyncEdMode::CheckActorChanged(AActor* Actor, const FString& Hash)
{
	FString NewSerialization = SerializeActor(Actor);
	if (NewSerialization.Left(NewSerialization.Len() - 1) != Hash)
	{
		SendChange(NewSerialization);
		// *LastActorMod.Find(Actor) = NewSerialization.Left(NewSerialization.Len() - 1);
	}
}

void FMapSyncEdMode::SendNewActor(AActor* NewActor)
{
	if(NewActor)
	{
		UE_LOG(LogMapSync, Log, TEXT("MapSync: Sending actor spawning -> %s"), *NewActor->GetFName().ToString());

		FString FormatedStr = GetWorld()->GetFName().ToString() + " create " + NewActor->GetFName().ToString() + " " + NewActor->GetClass()->GetFName().ToString() + ";";
		const TCHAR* SerializedChar = FormatedStr.GetCharArray().GetData();
		int32 Size = FCString::Strlen(SerializedChar);
		int32 Sent = 0;
		ConnectionToServer->Send((uint8*)(TCHAR_TO_UTF8(SerializedChar)), Size, Sent);
	}
}

void FMapSyncEdMode::SendDelActor(AActor* DelActor)
{
	UE_LOG(LogMapSync, Log, TEXT("MapSync: Sending actor deletion -> %s"), *DelActor->GetFName().ToString());

	FString FormatedStr = GetWorld()->GetFName().ToString() + " delete " + DelActor->GetFName().ToString() + ";";
	const TCHAR* SerializedChar = FormatedStr.GetCharArray().GetData();
	int32 Size = FCString::Strlen(SerializedChar);
	int32 Sent = 0;
	ConnectionToServer->Send((uint8*)(TCHAR_TO_UTF8(SerializedChar)), Size, Sent);
}

void FMapSyncEdMode::DeserializeNewActor(const FString& Serialization)
{
	TArray<FString> Parts;
	Serialization.ParseIntoArray(Parts, TEXT(" "));
	if (Parts.Num() >= 2)
	{
		for (TObjectIterator<UClass> Itr; Itr; ++Itr)
		{
			if (Itr->GetFName().ToString() == Parts[1])
			{
				FActorSpawnParameters sp;
				sp.Name = FName(*Parts[0]);
				AActor* Spawned = GetWorld()->SpawnActor(*Itr, nullptr, sp);
				LastActorMod.Add(TPairInitializer<AActor*, FString>(Spawned, SerializeActor(Spawned)));
				UE_LOG(LogMapSync, Log, TEXT("MapSync: Received actor spawn -> creating %s of class %s"), *Parts[0], *Parts[1]);
				break;
			}
		}
	}
}

void FMapSyncEdMode::DeserializeDelActor(const FString& Serialization)
{
	TArray<FString> Parts;
	Serialization.ParseIntoArray(Parts, TEXT(" "));
	if (Parts.Num() >= 1)
	{
		for (TActorIterator<AActor> Itr(GetWorld()); Itr; ++Itr)
		{
			if (Itr->GetFName().ToString() == Parts[0])
			{
				UE_LOG(LogMapSync, Log, TEXT("MapSync: Received actor deletion -> deleting %s"), *Itr->GetFName().ToString());

				Itr->Destroy();
				for (auto& ActorMod : LastActorMod)
				{
					if (ActorMod.Key == *Itr)
					{
						ActorMod.Value = "removed";
					}
				}
				break;
			}
		}
	}
}
