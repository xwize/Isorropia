#include "Isorropia.h"

#include <AclAPI.h>

const int PATCH_LEN = 14;
const unsigned __int64 PATCH_OFFSET = 0xBDADC66;

extern "C" void Payload();
extern "C" unsigned __int64 payloadResult;

void* PatchAddress() {
	unsigned __int64 addr = (unsigned __int64)GetModuleHandle(NULL);
	addr += PATCH_OFFSET;
	return (void*)addr;
}

void Isorropia::Patch() {
	LOG("Creating patch...\n");
	unsigned __int8 patch[PATCH_LEN] = //{ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 , 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90  };
	{ 0x48, 0xB8, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0xFF, 0xD0, 0x90, 0x90 };
	unsigned __int64* pPatchDestAddr = (unsigned __int64*)&patch[2];
	*pPatchDestAddr = (unsigned __int64)&Payload; //0x123456789ABCDEF;

	for (int i = 0; i < PATCH_LEN; ++i) {
		DebugPrintf("%x ", patch[i]);
	}

	LOG("Installing patch...\n");
	void* pPatchAddr = PatchAddress();
	DWORD dwOldProtect = 0;
	(void)VirtualProtect(pPatchAddr, PATCH_LEN, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	memcpy(pPatchAddr, patch, PATCH_LEN);
	(void)VirtualProtect(pPatchAddr, PATCH_LEN, dwOldProtect, &dwOldProtect);
}

microsecond_t Clock() {
	static LARGE_INTEGER pf;
	static bool firstTime = true;

	if (firstTime) {
		timeBeginPeriod(1);
		QueryPerformanceFrequency(&pf);
		firstTime = false;
	}

	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);

	double m = (10e5 * (double)li.QuadPart / (double)pf.QuadPart);
	return (microsecond_t)m;
}

void Block(microsecond_t x) {
	microsecond_t t = Clock();
	while ((Clock() - t) < x) {}
}

#if 0

float* Isorropia::_ResolutionModifierPtr() const {

	const int NUM_OFFSETS = 2;
	unsigned __int64 offset[NUM_OFFSETS] = { 0x60, 0xA4 };

	unsigned __int64 base = (unsigned __int64)BaseAddress();
	unsigned __int64 ptr = base + 0x053CDEB0;

	for (int i = 0; i < NUM_OFFSETS; ++i) {
		//const int BUF_LEN = 256;
		//char buf[BUF_LEN];
		//sprintf_s(buf, BUF_LEN, "%llx -> ", ptr);
		//DebugPrint(buf);
		unsigned __int64 addr64 = *(unsigned __int64*)ptr;
		ptr = addr64 + offset[i];
		///sprintf_s(buf, BUF_LEN, "%llx\n", ptr);
		///DebugPrint(buf);
	}
	
	return (float*)ptr;
}


float* Isorropia::_AdaptiveAATargetFPSPtr() const {

	const int NUM_OFFSETS = 4;
	unsigned __int64 offset[NUM_OFFSETS] = { 0x60, 0x970, 0x148, 0x38 };

	unsigned __int64 base = (unsigned __int64)BaseAddress();
	unsigned __int64 ptr = base + 0x053CDEB0;

	for (int i = 0; i < NUM_OFFSETS; ++i) {
		unsigned __int64 addr64 = *(unsigned __int64*)ptr;
		ptr = addr64 + offset[i];
	}

	return (float*)ptr;
}


float* Isorropia::_AdaptiveAATargetFPSPtr() const {

	const int NUM_OFFSETS = 4;
	unsigned __int64 offset[NUM_OFFSETS] = { 0x60, 0x970, 0x148, 0x38 };

	unsigned __int64 base = (unsigned __int64)BaseAddress();
	unsigned __int64 ptr = base + 0x053CDEB0;

	for (int i = 0; i < NUM_OFFSETS; ++i) {
		unsigned __int64 addr64 = *(unsigned __int64*)ptr;
		ptr = addr64 + offset[i];
	}

	return (float*)ptr;
}

float Isorropia::ResolutionModifier() const {
	return (*_ResolutionModifierPtr());
}

void Isorropia::SetResolutionModifier(const float& value) {
	const float minVal = 0.5f;
	const float maxVal = 2.0f;
	float v = max(minVal, min(maxVal, value));
	float* rmp = _ResolutionModifierPtr();
	*rmp = v;
}

float Isorropia::AdaptiveAATargetFPS() const {
	return (*_AdaptiveAATargetFPSPtr());
}

void Isorropia::SetAdaptiveAATargetFPS(const float& value) {
	const float minVal = 0.0f;
	const float maxVal = 240.0f;
	float v = max(minVal, min(maxVal, value));
	*_AdaptiveAATargetFPSPtr() = value;
}
#endif

Isorropia::Isorropia() {
	frame = 0;
	memset(usages, 0, sizeof(usages));
	usagesWriteIndex = 0;
	usagesSize = 0;
	clockPostPresent = 0;
	frameTimeMS = 1.0f;
}

err_t Isorropia::Init() {
	LOG("Isorropia. Created and developed by xwize 2018.\n");
	err_t err = gpuPerfMonitor.Init();
	if (err != ERR_NONE) {
		LOG("Couldn't initialise GPU performance monitor.\n");
		return err;
	} else {
		LOG("Isorropia initialised.\n");
	}
	return ERR_NONE;
}

void Isorropia::Shutdown() {
}

void* Isorropia::BaseAddress() const {
	return (void*)GetModuleHandle(NULL);
}

void Isorropia::_ComputeUsageStats() {
	// Compute the min, max
	usagesMax = 0;
	usagesMin = 0xFFFF;
	for (size_t i = 0; i < usagesSize; ++i) {
		usagesMax = max(usagesMax, usages[i]);
		usagesMin = min(usagesMin, usages[i]);
	}
}

void Isorropia::_ClearUsageMeasurements() {
	usagesWriteIndex = 0;
	usagesSize = 0;
}

void Isorropia::_AddUsageMeasurement(const uint32_t& sample) {
	usages[usagesWriteIndex] = sample;
	usagesWriteIndex = (usagesWriteIndex + 1) % ISORROPIA_MAX_USAGE_WINDOW_SIZE;
	usagesSize = min(usagesSize + 1, ISORROPIA_MAX_USAGE_WINDOW_SIZE);
}

void Isorropia::_SetupProcessPriority() {
	HANDLE p = GetCurrentProcess();;
	if (!SetPriorityClass(p, ABOVE_NORMAL_PRIORITY_CLASS)) {
		DebugPrintf("Failed to change process priority class.\n");
	}

	HANDLE  x = GetCurrentThread();
	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL)) {
		DebugPrintf("Failed to change thread priority.\n");
	}
}

void Isorropia::PrePresent(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT* pSyncIntervalOut) {

	static bool firstRun = false;

	static ID3D11Device* pDevice = NULL;
	static ID3D11DeviceContext* pContext = NULL;

	if (!firstRun) {
		// Change process priority
		//_SetupProcessPriority();

		pSwapChain->GetDevice(__uuidof(pDevice), (void**)&pDevice);
		pDevice->GetImmediateContext(&pContext);
		firstRun = true;

		Patch();
	}

	// Force no sync
	// *pSyncIntervalOut = 0;
}

void Isorropia::PostPresent() {

	// Desirables:
	// 1. Unnoticable
	// 2. Run at high resolution when possible
	// 3. Change as infrequently as possible
	// 4. Reacts as fast as possible

	// Observations:
	// 1. At night, torch causes massive usage spike

	// Design Stages:
	// 1. Measurement
	// 2. Control
	if (frame % ISORROPIA_FRAMES_PER_TICK == 0) {
		//_Measure();
		_Control();
		tick++;
	}

	microsecond_t clock = Clock();
	microsecond_t frameTime = clock - clockPostPresent;
	clockPostPresent = clock;
	frameTimeMS = (float)frameTime / (float)1000.0f;

	if (frame % ISORROPIA_FRAMES_PER_PRINT == 0) {
#ifdef ISORROPIA_DEBUG
		//DebugPrintf("Frame: %d. GPU Usage: %d [%d,%d]. FT: %f. ResMod: %.2f. TFPS: %.2f\n",
		//	frame, gpuPerfMonitor.Usage(), usagesMin, usagesMax, frameTimeMS, ResolutionModifier(), AdaptiveAATargetFPS());
#endif
	}
	frame++;
}

void Isorropia::_Measure() {
	// Get the GPU usage
	gpuPerfMonitor.Update();
	// Populate the usage window
	_AddUsageMeasurement(gpuPerfMonitor.Usage());
	/// Calculate the stats
	_ComputeUsageStats();
}

void Isorropia::_Control() {

#if 0
	const int numSetPoints = 4;
	const int estFramesPerSecond = ISORROPIA_TARGET_FPS;
	const int estSecondsPerTick = estFramesPerSecond / ISORROPIA_FRAMES_PER_TICK;
	
	const float AATargetAd = ISORROPIA_TARGET_FPS;
	const float AATargetLo = 100.0f;
	const float AATargetHi = 1.0f;

	const float resMods[numSetPoints]	= { 0.85f, 0.9f, 1.0f, 1.0f };
	const float AATargets[numSetPoints] = { AATargetLo, AATargetLo, AATargetLo, AATargetHi };

	const static int32_t drpLockTicks = (4 * estSecondsPerTick);
	const static int32_t adjLockTicks = (10 * estSecondsPerTick);

	static int32_t adjLock = 0;
	static int32_t drpLock = 0;
	static int32_t setPoint = 0;

	const float maxEmergencyLatenessMS = 1.75f;
	const float targetFrameTimeMS = 1.0f / (float)ISORROPIA_TARGET_FPS;
	float latenessMS = frameTimeMS - targetFrameTimeMS;
	bool wasLate = latenessMS > maxEmergencyLatenessMS;

	if (drpLock < 1) {
		if (gpuPerfMonitor.Usage() >= 98 && wasLate) {
			drpLock = drpLockTicks;
			adjLock = adjLockTicks;
			setPoint = min(setPoint - 2, numSetPoints - 1);
			setPoint = max(setPoint, 0);
			SetResolutionModifier(resMods[setPoint]);
			SetAdaptiveAATargetFPS(AATargets[setPoint]);
			_ClearUsageMeasurements();
			LOG("Emergency. Dropping settings.");
		}
	}

	if (adjLock < 1) {
		if (usagesMax >= 95) {
			adjLock = adjLockTicks;
			setPoint = min(setPoint - 1, numSetPoints - 1);
			setPoint = max(setPoint, 0);
			SetResolutionModifier(resMods[setPoint]);
			SetAdaptiveAATargetFPS(AATargets[setPoint]);
			_ClearUsageMeasurements();
			LOG("Decreasing settings.");
		} else if (usagesMax < 65) {
			adjLock = adjLockTicks;
			setPoint = min(setPoint + 2, numSetPoints - 1);
			setPoint = max(setPoint, 0);
			SetResolutionModifier(resMods[setPoint]);
			SetAdaptiveAATargetFPS(AATargets[setPoint]);
			_ClearUsageMeasurements();
			LOG("Increasing settings x2.");
		} else if (usagesMax < 75) {
			adjLock = adjLockTicks;
			setPoint = min(setPoint + 1, numSetPoints - 1);
			setPoint = max(setPoint, 0);
			SetResolutionModifier(resMods[setPoint]);
			SetAdaptiveAATargetFPS(AATargets[setPoint]);
			_ClearUsageMeasurements();
			LOG("Increasing settings.");
		}
	}

	adjLock = max(adjLock - 1, 0);
	drpLock = max(drpLock - 1, 0);
#endif

	if (GetAsyncKeyState(VK_DOWN)) {
		//SetResolutionModifier(0.5f);
		//SetAdaptiveAATargetFPS(10.0f);
	}

	DebugPrintf("%llx\n", payloadResult);
}