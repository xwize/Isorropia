#ifndef __GPU_PERF_H__
#define __GPU_PERF_H__

#include "Common.h"

class GPUPerfMonitor {
	uint32_t	usage;
public:
				GPUPerfMonitor();
	err_t		Init();
	void		Update();
	uint32_t	Usage() const;
};

#endif /// __GPU_PERF_H__