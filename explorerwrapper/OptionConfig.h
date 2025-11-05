#pragma once
#include "common.h"
#include "dbgprint.h"
#include "OSVersion.h"
#include "RegistryManager.h"

// Individual option definitions
// - These are external to ensure we can call them elsewhere
extern bool s_ClassicTheme;
extern bool s_DisableComposition;
extern bool s_UseTaskbarPinning;
extern bool s_ShowStoreAppsInStart;
extern int s_ColorizationOptions;
extern bool s_OverrideAlpha;
extern DWORD s_AlphaValue;

// Responsible for settings these values and calling them from registry
extern void InitializeConfiguration();
