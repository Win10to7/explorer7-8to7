#pragma once
#include "common.h"
#include "dbgprint.h"
#include "OptionConfig.h"
#include "OSVersion.h"

// Ittr: Pattern imports are defined and rewritten here rather than dllmain. Feel free to further improve this.

// Allow 7 msstyles to load by removing animation map data from uxtheme.dll imports
void RemoveLoadAnimationDataMap();
void RemoveGetClassIdForShellTarget(); // for Windows 8.1

// Fix authui.dll import for CLogOffOptions by replacing bytes
void FixAuthUI();

// Remove unwanted immersive shell interfaces, often by preventing them from running
void RestoreWin32Menus();

// Forcefully enable a conditional check to allow SetWindowRgn to be applied at the right time
void RepairRegionBehaviour();

// Main procedure we call from elsewhere
void ChangePatternImports();
