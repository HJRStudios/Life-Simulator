// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "Editor/UnrealEd/Public/Features/IPluginsEditorFeature.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMapSync, Log, All);

class FMapSyncModule : public IModuleInterface, public IPluginsEditorFeature
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

};
