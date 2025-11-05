#include "PatternImports.h"

// Remove AMAP class from loaded msstyle so that Vista and 7 msstyles are compatible
void RemoveLoadAnimationDataMap()
{
	// thank you amrsatrio for the pattern + offsetting method
	char* LoadAnimationDataMap = "48 8B 53 20 48 8B ?? E8 ?? ?? ?? ?? 8B ?? 48 8B";

	HMODULE uxTheme = GetModuleHandle(L"uxtheme.dll");
	if (uxTheme)
	{
		char* LADMPattern = (char*)FindPattern((uintptr_t)uxTheme, LoadAnimationDataMap);

		if (LADMPattern)
		{
			LADMPattern += 7;
			LADMPattern += 5 + *(int*)(LADMPattern + 1);

			unsigned char bytes[] = { 0x31, 0xC0, 0xC3 };
			ChangeImportedPattern(LADMPattern, bytes, sizeof(bytes));
		}
	}
}

// For Windows 8.1 - remove additional Immersive class from loaded msstyle so that Vista and 7 msstyles are compatible
void RemoveGetClassIdForShellTarget()
{
	char* GetClassIdForShellTarget = "4C 8B DC 4D 89 43 18 49 89 4B 08 53 48 83 EC 30";

	HMODULE uxTheme = GetModuleHandle(L"uxtheme.dll");
	if (uxTheme)
	{
		char* GCIFSTPattern = (char*)FindPattern((uintptr_t)uxTheme, GetClassIdForShellTarget);

		if (GCIFSTPattern)
		{
			unsigned char bytes[] = { 0x31, 0xC0, 0xC3 };
			ChangeImportedPattern(GCIFSTPattern, bytes, sizeof(bytes));
		}
	}
}

// Fix CLogoffPane so that options are correctly displayed
void FixAuthUI()
{
	// Newer explorer versions use this
	// CLogoffPane::_InitShutdownObjects
	const char* bytes = "48 8B ?? 98 00 00 00 48 8B ?? 40 45 33 C0 48 8B 01 FF 50 18 "
		"48 8B ?? 98 00 00 00 48 8B 01 FF 50 30 8B D8 "
		"85 C0 ?? ?? "
		"48 8B ?? 98 00 00 00 48 8D ?? A0 00 00 00 48 8B 01 FF 50 20";

	// Older explorer versions use this
	// CLogoffPane::_OnCreate
	const char* bytesOld = "48 8B 8B 98 00 00 00 48 8B 53 40 45 33 C0 48 8B 01 FF 50 18 "
		"48 8B 8B 98 00 00 00 48 8B 01 FF 50 30 44 8B C8 "
		"85 C0 ?? ?? ?? ?? ?? ?? "
		"48 8B 8B 98 00 00 00 48 8D 93 A0 00 00 00 48 8B 01 FF 50 20";

	char* pattern = (char*)FindPattern((uintptr_t)GetModuleHandle(NULL), bytes);
	char* pattern1 = (char*)FindPattern((uintptr_t)GetModuleHandle(NULL), bytesOld);

	unsigned char patch1[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
	SIZE_T size = sizeof(patch1);
	if (pattern)
	{
		// mov rax, [rcx]
		// call qword ptr [rax+18h]
		char* inst1 = pattern + 14;
		ChangeImportedPattern(inst1, patch1, size);

		// mov rax, [rcx]
		// call qword ptr [rax+30h]
		char* inst2 = pattern + 27;
		ChangeImportedPattern(inst2, patch1, size);

		// mov rax, [rcx]
		// call qword ptr [rax+20h]
		char* inst3 = pattern + 53;
		ChangeImportedPattern(inst3, patch1, size);
	}

	if (pattern1 && !pattern) // Ittr: Only apply to CLogoffPane::_OnCreate if we need to, otherwise this causes crashing on later 7 explorer.
	{
		// mov rax, [rcx]
		// call qword ptr [rax+18h]
		char* inst1 = pattern1 + 14;
		ChangeImportedPattern(inst1, patch1, size);

		// mov rax, [rcx]
		// call qword ptr [rax+30h]
		char* inst2 = pattern1 + 27;
		ChangeImportedPattern(inst2, patch1, size);

		// mov rax, [rcx]
		// call qword ptr [rax+20h]
		char* inst3 = pattern1 + 58;
		ChangeImportedPattern(inst3, patch1, size);
	}
}

// Ittr: Remove broken leftovers of immersive context menus, starting in Windows 10.
// Also to be noted that Windows 11 makes further changes here that we have to account for.
void RestoreWin32Menus()
{
	// Refactor: Do this in two separate parts - more readable but more lines of code (compiler should optimise...)
	// Pattern variables initialised locally in both to prevent race-conditions
	char unsigned bytes[] = { 0xB0, 0x00, 0xC3 }; // mov al 0, retn - running retn on its own here has inconsistent results
	
	// SHELL32
	HMODULE shell32 = GetModuleHandle(L"shell32.dll");

	if (shell32)
	{
		char* CanApplyOwnerDrawToMenu = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 70 33 DB 48 8B F2 33 FF 48 8B E9";
		char* CAODTMPattern = (char*)FindPattern((uintptr_t)shell32, CanApplyOwnerDrawToMenu);

		if (CAODTMPattern) // 24H2 and later
		{
			ChangeImportedPattern(CAODTMPattern, bytes, sizeof(bytes));
		}
		else
		{
			char* CanApplyOwnerDrawToMenu = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 70 48 8B F2 48 8B E9 33 FF 33 D2";
			char* CAODTMPattern = (char*)FindPattern((uintptr_t)shell32, CanApplyOwnerDrawToMenu);

			if (CAODTMPattern) // TH1 to 23H2
			{
				ChangeImportedPattern(CAODTMPattern, bytes, sizeof(bytes));
			}
		}
	}

	// EXPLORERFRAME
	HMODULE explorerFrame = LoadLibrary(L"ExplorerFrame.dll");

	if (explorerFrame)
	{
		char* CanApplyOwnerDrawToMenu = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 70 33 DB 48 8B F2 33 FF 48 8B E9";
		char* CAODTMPattern = (char*)FindPattern((uintptr_t)explorerFrame, CanApplyOwnerDrawToMenu);

		if (CAODTMPattern) // 24H2 and later
		{
		}
		else
		{
			char* CanApplyOwnerDrawToMenu = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 70 48 8B F2 48 8B E9 33 FF 33 D2 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? C7 44 24 20 50 00 00 00";
			char* CAODTMPattern = (char*)FindPattern((uintptr_t)explorerFrame, CanApplyOwnerDrawToMenu);

			if (CAODTMPattern) // 21H2 to 23H2 (i think... i forgot to comment this properly at the time)
			{
			}
			else
			{
				char* CanApplyOwnerDrawToMenu = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 70 48 8B F2 48 8B E9 33 FF 33 D2";
				char* CAODTMPattern = (char*)FindPattern((uintptr_t)explorerFrame, CanApplyOwnerDrawToMenu);

				if (CAODTMPattern) // TH1 to VB
				{
				}
			}
		}
	}
}

// Ensure that the start menu region is corrected as applicable
void RepairRegionBehaviour()
{
	// For most versions this involves needing to change an conditional jump to an unconditional jump
	char* _ReapplyRegionConditional = "44 38 AB 78 08 00 00 0F 84 E6 00 00 00";
	char* RRCPattern = (char*)FindPattern((uintptr_t)GetModuleHandle(NULL), _ReapplyRegionConditional);

	if (RRCPattern)
	{
		unsigned char bytes[] = { 0x44, 0x38, 0xAB, 0x78, 0x08, 0x00, 0x00, 0xE9, 0xE7, 0x00, 0x00, 0x00, 0x90 };
		ChangeImportedPattern(RRCPattern, bytes, sizeof(bytes));
	}
	else
	{
		// In this case the jump is flipped, so it has to be skipped over
		_ReapplyRegionConditional = "44 38 B3 78 08 00 00 0F 85 ?? ?? ?? ?? FF";
		RRCPattern = (char*)FindPattern((uintptr_t)GetModuleHandle(NULL), _ReapplyRegionConditional);

		if (RRCPattern)
		{
			unsigned char bytes[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xFF };
			ChangeImportedPattern(RRCPattern, bytes, sizeof(bytes));
		}
	}
}

void ChangePatternImports()
{
	// Remove Windows 8+ animation msstyle classes so that legacy msstyles from Vista onwards are compatible with our theming system
	RemoveLoadAnimationDataMap();
	RemoveGetClassIdForShellTarget();

	// Responsible for fixing CLogoffOptions
	FixAuthUI();
	
	// Disable various unwanted immersive interfaces
	RestoreWin32Menus(); // Remove the immersive menu leftovers so that the taskbar behaves properly in accordance with Windows 7

	// Amend some code behaviour so the start menu expand animation behaves more predictably
	RepairRegionBehaviour();
}