#include "Isorropia.h"

#pragma pack(1)

#define DXGI_DLL			"dxgi.dll"
#define DXGI_LINKED_DLL		"dxgi_linked.dll"

FARPROC p[9] = { 0 };

static Isorropia isor;
static ID3D11Device* pD3D11Device = NULL;

extern "C" BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
	static HINSTANCE hL;
	if (reason == DLL_PROCESS_ATTACH) {
		// Load the original file from System32
		const int PATH_LEN = 512;
		TCHAR path[PATH_LEN];
		GetSystemDirectory(path, PATH_LEN);
		hL = LoadLibrary(lstrcat(path, _T("\\" DXGI_DLL)));

		if (!hL) {
			assert(false);
			return FALSE;
		}

		// Grab the addresses
		p[0] = GetProcAddress(hL, "CreateDXGIFactory");
		p[1] = GetProcAddress(hL, "CreateDXGIFactory1");
		p[2] = GetProcAddress(hL, "CreateDXGIFactory2");
		p[3] = GetProcAddress(hL, "DXGID3D10CreateDevice");
		p[4] = GetProcAddress(hL, "DXGID3D10CreateLayeredDevice");
		p[5] = GetProcAddress(hL, "DXGID3D10GetLayeredDeviceSize");
		p[6] = GetProcAddress(hL, "DXGID3D10RegisterLayers");
		p[7] = GetProcAddress(hL, "DXGIDumpJournal");
		p[8] = GetProcAddress(hL, "DXGIReportAdapterConfiguration");

		for (int i = 0; i < 9; ++i) {
			assert(p[i] != NULL);
		}

		// Replaces prologue of functions with jmps to its own hooks
		HINSTANCE hLL = LoadLibrary(_T(DXGI_LINKED_DLL));

		// Init the controller
		isor.Init();
	}

	if (reason == DLL_PROCESS_DETACH) {
		FreeLibrary(hL);
	}

	return TRUE;
}

typedef HRESULT(__stdcall *CreateSwapChainPtr)(IDXGIFactory* pThis,
	_In_  IUnknown *pDevice, _In_  DXGI_SWAP_CHAIN_DESC *pDesc, _Out_  IDXGISwapChain **ppSwapChain);

typedef HRESULT(__stdcall *CreateSwapChain1Ptr)(IDXGIFactory1* pThis,
	_In_  IUnknown *pDevice, _In_  DXGI_SWAP_CHAIN_DESC *pDesc, _Out_  IDXGISwapChain **ppSwapChain);

typedef HRESULT(__stdcall *PresentPtr)(IDXGISwapChain* pThis,
	UINT, UINT);

CreateSwapChain1Ptr CreateSwapChain1_original = NULL;
PresentPtr Present_original = NULL;

HRESULT __stdcall Hooked_IDXGISwapChain_Present(IDXGISwapChain* pThis, UINT syncInterval, UINT flags) {
	TRACE("> Hooked_IDXGISwapChain_Present");

	UINT newSyncInterval = 0;
	UINT newFlags;
	isor.PrePresent(pThis,syncInterval,&newSyncInterval);
	HRESULT hr = Present_original(pThis, newSyncInterval, flags);
	isor.PostPresent();
	return hr;
}

void Hook_IDXGISwapChain_Present(IDXGISwapChain* pBase) {
	TRACE("> Hook_IDXGISwapChain_Present");
	const int idx = 8;
	unsigned __int64 *pVTableBase = (unsigned __int64 *)(*(unsigned __int64 *)pBase);
	unsigned __int64 *pVTableFnc = (unsigned __int64 *)((pVTableBase + idx));
	void *pHookFnc = (void *)Hooked_IDXGISwapChain_Present;

	Present_original = (PresentPtr)pVTableBase[idx];

	DWORD dwOldProtect = 0;
	(void)VirtualProtect(pVTableFnc, sizeof(void *), PAGE_EXECUTE_READWRITE, &dwOldProtect);
	memcpy(pVTableFnc, &pHookFnc, sizeof(void *));
	(void)VirtualProtect(pVTableFnc, sizeof(void *), dwOldProtect, &dwOldProtect);
}

HRESULT __stdcall Hooked_IDXGIFactory1_CreateSwapChain(IDXGIFactory1* pThis,
	_In_  IUnknown *pDevice, _In_  DXGI_SWAP_CHAIN_DESC *pDesc, _Out_  IDXGISwapChain **ppSwapChain) {
	TRACE("> Hooked_IDXGIFactory1_CreateSwapChain");

	assert(pDesc != NULL);
	// Call original
	assert(CreateSwapChain1_original != NULL);
	HRESULT res = CreateSwapChain1_original(pThis, pDevice, pDesc, ppSwapChain);

	// Hook Present
	if (SUCCEEDED(res) && pDevice != NULL && Present_original == NULL) {
		Hook_IDXGISwapChain_Present(*ppSwapChain);
	}

	return res;
}


void Hook_IDXGIFactory1_CreateSwapChain(IDXGIFactory1* pBase) {
	TRACE("> Hook_IDXGIFactory1_CreateSwapChain");
	assert(pBase != NULL);
	const __int64 idx = 10;
	unsigned __int64 *pVTableBase = (unsigned __int64 *)(*(unsigned __int64 *)pBase);
	unsigned __int64 *pVTableFnc = (unsigned __int64 *)((pVTableBase + idx));
	void *pHookFnc = (void *)Hooked_IDXGIFactory1_CreateSwapChain;

	// Stop recursion if we've already hooked the vtable here
	if ((CreateSwapChainPtr)pVTableBase[idx] == pHookFnc) {
		LOG("IDXGIFactory1_CreateSwapChain already hooked. Skipping.\n");
		return;
	}

	CreateSwapChain1_original = (CreateSwapChain1Ptr)pVTableBase[idx];

	DWORD dwResult = 0;
	DWORD dwOldProtect = 0;
	dwResult = VirtualProtect(pVTableFnc, sizeof(void *), PAGE_EXECUTE_READWRITE, &dwOldProtect);
	assert(dwResult != 0);
	memcpy(pVTableFnc, &pHookFnc, sizeof(void *));
	dwResult = VirtualProtect(pVTableFnc, sizeof(void *), dwOldProtect, &dwOldProtect);
	assert(dwResult != 0);
}

// ------------------------ Proxies ------------------------

extern "C" long Proxy_CreateDXGIFactory(REFIID riid, void** ppFactory) {
	TRACE("> Proxy_CreateDXGIFactory");
	typedef long(__stdcall *pS)(REFIID, void**);

	if (riid == __uuidof(IDXGIFactory2)) {
		return E_NOINTERFACE;
	}

	pS pps = (pS)(p[0]);
	long rv = pps(riid, ppFactory);
	return rv;
}

extern "C" long Proxy_CreateDXGIFactory1(REFIID riid, void** ppFactory) {
	TRACE("> Proxy_CreateDXGIFactory1");
	typedef long(__stdcall *pS)(REFIID, void**);

	if (riid == __uuidof(IDXGIFactory2)) {
		return E_NOINTERFACE;
	}

	pS pps = (pS)(p[1]);
	long rv = pps(riid, ppFactory);

	// VMT hook the DXGIFactory1
	IDXGIFactory1* pFactory = (IDXGIFactory1*)(*ppFactory);
	if (pFactory != NULL) {
		Hook_IDXGIFactory1_CreateSwapChain(pFactory);
	}
	return rv;
}

extern "C" long Proxy_CreateDXGIFactory2(unsigned int  a1, __int64 a2, __int64 *a3) {
	TRACE("> Proxy_CreateDXGIFactory2");
	typedef long(__stdcall *pS)(unsigned int  a1, __int64 a2, __int64 *a3);
	pS pps = (pS)(p[2]);
	long rv = pps(a1, a2, a3);
	return rv;
}

extern "C" long Proxy_DXGID3D10CreateDevice(HMODULE hModule, IDXGIFactory *pFactory, IDXGIAdapter *pAdapter,
	UINT Flags, void *unknown, void *ppDevice) {
	TRACE("> Proxy_DXGID3D10CreateDevice");
	typedef long(__stdcall *pS)(HMODULE hModule, IDXGIFactory *pFactory, IDXGIAdapter *pAdapter,
		UINT Flags, void *unknown, void *ppDevice);
	pS pps = (pS)(p[3]);
	long rv = pps(hModule, pFactory, pAdapter, Flags, unknown, ppDevice);
	return rv;
}

struct UNKNOWN { BYTE unknown[20]; };
extern "C" long Proxy_DXGID3D10CreateLayeredDevice(UNKNOWN unknown) {
	typedef long(__stdcall *pS)(UNKNOWN);
	pS pps = (pS)(p[4]);
	long rv = pps(unknown);
	return rv;
}

extern "C" SIZE_T Proxy_DXGID3D10GetLayeredDeviceSize(const void *pLayers, UINT NumLayers) {
	typedef SIZE_T(__stdcall *pS)(const void *pLayers, UINT NumLayers);
	pS pps = (pS)(p[5]);
	SIZE_T rv = pps(pLayers, NumLayers);
	return rv;
}

extern "C" long Proxy_DXGID3D10RegisterLayers(const void *pLayers, UINT NumLayers) {
	typedef long(__stdcall *pS)(const void *pLayers, UINT NumLayers);
	pS pps = (pS)(p[6]);
	long rv = pps(pLayers, NumLayers);
	return rv;

}

extern "C" long Proxy_DXGIDumpJournal() {
	typedef long(__stdcall *pS)();
	pS pps = (pS)(p[7]);
	long rv = pps();
	return rv;
}

extern "C" long Proxy_DXGIReportAdapterConfiguration() {
	typedef long(__stdcall *pS)();
	pS pps = (pS)(p[8]);
	long rv = pps();
	return rv;
}