#pragma once
#include "util.h"
#include "common.h"
#include "dbgprint.h"
#include "OptionConfig.h"
#include "OSVersion.h"
#include "TypeDefinitions.h"
#include "MinHook.h"
#include "NscTree.h"

HTHEME __stdcall OpenThemeData_Hook(HWND hwnd, LPCWSTR pszClassList)
{
	if (g_dwTrayThreadId > 0 && g_dwTrayThreadId != GetCurrentThreadId())
		return fOpenThemeData(hwnd, pszClassList);

	if (!AllowThemes())
		return NULL;

	LoadCurrentTheme(hwnd, pszClassList);

	if (g_currentTheme == nullptr)
		dbgprintf(L"OPENTHEMEDATA FAILED %s", pszClassList);

	themeHandles->push_back(g_currentTheme);
	return g_currentTheme;
}

HTHEME __stdcall OpenThemeDataForDpi_Hook(HWND hwnd, LPCWSTR pszClassList, UINT dpi)
{
	if (g_dwTrayThreadId > 0 && g_dwTrayThreadId != GetCurrentThreadId())
		return fOpenThemeDataForDpi(hwnd, pszClassList, dpi);

	if (!AllowThemes())
		return NULL;

	HTHEME theme = 0;
	DWORD flags = 2;
	if (dpi != 96)
		flags |= 1u;

	// Ittr: Windows 11 introduces issues with applying themes to the SearchFolder interface, due to the ItemsViewAccessible::Header addition
	// This is resolved by simply falling back to the system theme if the DirectUI theme call attempts to load this class
	if (g_loadedTheme && (lstrcmp(pszClassList, L"ItemsViewAccessible::Header") != 0))
	{
		theme = OpenThemeDataFromFile(g_loadedTheme, hwnd, pszClassList, flags);
	}
	else
	{
		theme = fOpenThemeDataForDpi(hwnd, pszClassList, dpi);
	}

	if (theme == nullptr)
		dbgprintf(L"OPENTHEMEDATAFORDPI FAILED %s", pszClassList);
	themeHandles->push_back(theme);
	return theme;
}

HTHEME __stdcall OpenThemeDataEx_Hook(HWND hwnd, LPCWSTR pszClassList, DWORD dwFlags)
{
	if (g_dwTrayThreadId > 0 && g_dwTrayThreadId != GetCurrentThreadId())
		return fOpenThemeDataEx(hwnd, pszClassList, dwFlags);

	if (!AllowThemes())
		return NULL;

	HTHEME theme = 0;
	DWORD flags = 2;
	if ((unsigned int)GetScreenDpi() != 96)
		flags |= 1u;

	if (g_loadedTheme)
		theme = OpenThemeDataFromFile(g_loadedTheme, hwnd, pszClassList, dwFlags | flags);
	else
		theme = fOpenThemeDataEx(hwnd, pszClassList, dwFlags);

	if (theme == nullptr)
		dbgprintf(L"OPENTHEMEDATAEX FAILED %s", pszClassList);
	themeHandles->push_back(theme);
	return theme;
}

void RenderThumbnail(PVOID This, int animoffset, int bNoRedraw)
{
	RECT rc = *(RECT*)((PBYTE)This + 0x68);
	HWND hwnd = *(HWND*)((PBYTE)This + 0x60);
	HTHEME hthem = *(HTHEME*)((PBYTE)This + 0x98);

	renderThumbnail_orig(This, animoffset, bNoRedraw);

	MARGINS mar;
	GetThemeMargins(hthem, NULL, 2, 0, TMT_CONTENTMARGINS, NULL, &mar);
	rc.left += mar.cxLeftWidth;
	rc.right -= mar.cxRightWidth;
	rc.top += mar.cyTopHeight;
	rc.bottom -= mar.cyBottomHeight;
	DwmpUpdateAccentBlurRect(hwnd, &rc);
}

HICON GetUWPIcon(HWND a2)
{
	HICON icon = NULL;
	IShellItemImageFactory* psiif = nullptr;
	IPropertyStore* ips;
	SHGetPropertyStoreForWindow(a2, IID_PPV_ARGS(&ips));
	GUID myGuid = { 0x9F4C2855, 0x9F79, 0x4B39, {0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3} };
	PROPERTYKEY propertyKey = { myGuid, 5 };
	PROPVARIANT pv;
	ips->GetValue(propertyKey, &pv);
	if (pv.vt == VT_LPWSTR)
	{
		LPCWSTR aumid = pv.pwszVal;
		SHCreateItemInKnownFolder(FOLDERID_AppsFolder, KF_FLAG_DONT_VERIFY, aumid, IID_PPV_ARGS(&psiif));
		if (psiif)
		{
			SIIGBF flags = SIIGBF_ICONONLY;
			HBITMAP hb;
			SIZE size = { GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CXICON) };
			HRESULT hr = psiif->GetImage(size, flags, &hb);
			if (SUCCEEDED(hr))
			{
				HIMAGELIST hImageList = ImageList_Create(size.cx, size.cy, ILC_COLOR32, 1, 0);
				if (ImageList_Add(hImageList, hb, NULL) != -1)
				{
					HICON hc = ImageList_GetIcon(hImageList, 0, 0);
					ImageList_Destroy(hImageList);

					// set
					icon = hc;

					DeleteObject(hb);
					psiif->Release();
				}
				DeleteObject(hb);
			}
			psiif->Release();
		}
	}
	ips->Release();
	return icon;
}

PVOID CTaskBandPtr = 0;

VOID CTaskBand_SetWindowIconHook(PVOID This, HWND a2, HICON a3, int a4)
{
	CTaskBandPtr = This;

	auto bIsImmersiveWnd = [](HWND hwnd) -> bool
		{
			return IsShellFrameWindow && IsShellFrameWindow(hwnd);
		};

	if (bIsImmersiveWnd(a2))
	{
		HICON icon = GetUWPIcon(a2);
		if (icon)
		{
			CTaskBand_SetWindowIconOrig(This, a2, icon, a4);
		}
	}
	else
	{
		CTaskBand_SetWindowIconOrig(This, a2, a3, a4);
	}
}

VOID UpdateItemIcon(PVOID This, int a2)
{
	typedef void* (__fastcall* GetTaskItemFunc)(void*);
	typedef HWND(__fastcall* GetWindowFunc)(void*);

	HDPA hdpaTaskThumbnails = *(HDPA*)((PBYTE)This + 0xB0);
	auto v4 = DPA_FastGetPtr(hdpaTaskThumbnails, a2);
	auto vtable = *(uintptr_t**)v4;
	GetTaskItemFunc GetTaskItem = (GetTaskItemFunc)vtable[0x60 / sizeof(uintptr_t)];
	void* v5 = GetTaskItem(v4);
	auto vtable_v5 = *(uintptr_t**)v5;
	GetWindowFunc GetWindow = (GetWindowFunc)vtable_v5[0x98 / sizeof(uintptr_t)];
	HWND v6 = GetWindow(v5);
	if (IsShellFrameWindow && IsShellFrameWindow(v6))
	{
		HICON hc = GetUWPIcon(v6);
		if (hc) SetIconThumb(This, hc, a2, 3);
	}
	else
		UpdateItem(This, a2);

}

// Ittr: Under immersive mode, the differences in ShellHook operation have to be accounted for
HRESULT(__fastcall* OnShellHookMessage)(void* a1);

bool fShowLauncher = false; // Ittr: First run erroneously shows the start menu, unless we handle it differently

HRESULT OnShellHookMessage_Hook(void* a1) // This gets called when start menu is to be opened - has been a bit temperamental
{
	// Use of fShowLauncher flag is essential to have something resembling stability for overall functionality
	if (fShowLauncher) // If the flag is set...
	{
		PostMessageW(hwnd_taskbar, 0x504, 0, 0); // Fire the message directly that opens Windows 7's start menu - ShellHook unreliable pre-VB
		return S_OK; // Ensure the run is recognised as a success 
	}
	else // However, without the flag, this is presumed to be the first run
	{
		fShowLauncher = true; // Enable the flag for showing the start menu now that this first attempt has run through
		return E_FAIL;  // Then, ensure this run is marked as a failure
	}

	return OnShellHookMessage(a1); // This codepath should ideally never run
}

void SetUpThemeManager()
{
	// Initialize the theme manager and declare the types for the UXTheme apis we're hooking
	ThemeManagerInitialize();

	fOpenThemeData = decltype(fOpenThemeData)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeData"));
	fOpenThemeDataForDpi = decltype(fOpenThemeDataForDpi)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeDataForDpi"));
	fOpenThemeDataEx = decltype(fOpenThemeDataEx)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeDataEx"));

	// Hook UXTheme-related calls for the purpose of our inactive theme system.
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeData), OpenThemeData_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeData));
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeDataForDpi), OpenThemeDataForDpi_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeDataForDpi));
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeDataEx), OpenThemeDataEx_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeDataEx));
}

void UpdateTrayWindowDefinitions()
{
	// Hook and update definitions of what windows should be added to the tray - largely for UWP purposes, but essentially zero-cost so included on both immersive on and off modes.
	void* _ShouldAddWindowToTray = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B F9 33 DB");
	void* _IsWindowNotDesktopOrTray = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 33 DB FF 15 ?? ?? ?? ?? 3B C3 74 ?? 48 3B 3D");
	MH_CreateHook(static_cast<LPVOID>(_ShouldAddWindowToTray), ShouldAddWindowToTray, reinterpret_cast<LPVOID*>(&_ShouldAddWindowToTray));
	MH_CreateHook(static_cast<LPVOID>(_IsWindowNotDesktopOrTray), IsWindowNotDesktopOrTray, reinterpret_cast<LPVOID*>(&_IsWindowNotDesktopOrTray));
}

void ChangeMinhookImports()
{
	MH_Initialize();

	SetUpThemeManager(); // Local visual style management init
	UpdateTrayWindowDefinitions(); // Ensure tray exclusion is corrected for modern Windows
}
