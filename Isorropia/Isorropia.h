#ifndef __ISORROPIA_H__
#define __ISORROPIA_H__

#include "Config.h"
#include "Common.h"
#include "GPUPerf.h"


class Isorropia {
private:
	float*				_AdaptiveAATargetFPSPtr() const;
	GPUPerfMonitor		gpuPerfMonitor;
	uint32_t			frame;
	uint32_t			tick;
	uint32_t			usages[ISORROPIA_MAX_USAGE_WINDOW_SIZE];
	uint32_t			usagesWriteIndex;
	uint32_t			usagesSize;
	uint32_t			usagesMax;
	uint32_t			usagesMin;
	microsecond_t		clockPostPresent;
	float				frameTimeMS;
	void				_ComputeUsageStats();
	void				_ClearUsageMeasurements();
	void				_AddUsageMeasurement(const uint32_t& sample);
	void				_SetupProcessPriority();
	void				_Measure();
	void				_Control();
public:
						Isorropia();

	err_t				Init();
	void				Shutdown();

	void				PrePresent(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT* pSyncIntervalOut);
	void				PostPresent();

	void*				BaseAddress() const;

	float				AdaptiveAATargetFPS() const;
	void				SetAdaptiveAATargetFPS(const float& value);
	//float				ResolutionModifier() const;
	//void				SetResolutionModifier(const float& value);

	void				Patch();
};

#endif // __ISORROPIA_H__
