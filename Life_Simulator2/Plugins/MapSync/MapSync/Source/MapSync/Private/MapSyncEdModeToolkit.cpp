// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MapSyncPrivatePCH.h"
#include "MapSyncEdMode.h"
#include "MapSyncEdModeToolkit.h"

#define LOCTEXT_NAMESPACE "FMapSyncEdModeToolkit"

FMapSyncEdModeToolkit::FMapSyncEdModeToolkit()
{
	struct Locals
	{
		static bool IsWidgetEnabled()
		{
			return GEditor->GetSelectedActors()->Num() != 0;
		}
	};

	const float Factor = 256.0f;
	SAssignNew(ToolkitWidget, SBorder)
	.VAlign(VAlign_Top)
	.Padding(15)
	.IsEnabled_Static(&Locals::IsWidgetEnabled)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text(LOCTEXT("HelperLabel", "\
To set IP and port of server to connect to, go into project folder, \
then into the Config folder, edit DefaultEngine.ini, and add at the end:\n \
[MapSync]\n \
Server=IP:port\n \
(you have to replace IP:port by your server's IP and port)\n \
If this is the first time you launch the engine with the plugin, you have to restart it, so you your log doesn't get spammed\n \
If you think there is an error, please consult log\n\n \
Using this plugin generates a huge amount of log, \
because it extensively uses the linker.\n \
So, LogLinker is automatically disabled. Same for LogNet:VeryVerbose"))
		]
	];
}

FName FMapSyncEdModeToolkit::GetToolkitFName() const
{
	return FName("MapSyncEdMode");
}

FText FMapSyncEdModeToolkit::GetBaseToolkitName() const
{
	return NSLOCTEXT("MapSyncEdModeToolkit", "DisplayName", "MapSyncEdMode Tool");
}

class FEdMode* FMapSyncEdModeToolkit::GetEditorMode() const
{
	return GLevelEditorModeTools().GetActiveMode(FMapSyncEdMode::EM_MapSyncEdModeId);
}

#undef LOCTEXT_NAMESPACE
