// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MapSyncPrivatePCH.h"
#include "MapSyncEdMode.h"
#include "SlateBasics.h"
#include "SlateStyle.h"
#include "EditorStyleSet.h"
#include "IPluginManager.h"

#define LOCTEXT_NAMESPACE "FMapSyncModule"

DEFINE_LOG_CATEGORY(LogMapSync);

void FMapSyncModule::StartupModule()
{
	FEditorModeRegistry::Get().RegisterMode<FMapSyncEdMode>(FMapSyncEdMode::EM_MapSyncEdModeId, LOCTEXT("MapSyncEdModeName", "MapSyncEdMode"), FSlateIcon(FEditorStyle::GetStyleSetName(), "DeviceDetails.Share"), true);
}

void FMapSyncModule::ShutdownModule()
{
	FEditorModeRegistry::Get().UnregisterMode(FMapSyncEdMode::EM_MapSyncEdModeId);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMapSyncModule, MapSync)
