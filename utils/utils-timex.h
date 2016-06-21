#pragma once
extern "C" {
#include <sys/time.h>
}

namespace utils {
	#define TIMEX_BGN(elapsed) \
		{ \
			struct timeval __t1, __t2; \
			gettimeofday(&__t1, NULL); \
			{

	#define TIMEX_END(elapsed) \
			} \
			gettimeofday(&__t2, NULL); \
			elapsed  = (__t2.tv_sec  - __t1.tv_sec) * 1000.0; \
			elapsed += (__t2.tv_usec - __t1.tv_usec) / 1000.0; \
		}
}
