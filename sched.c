#include <sched.h>
#include <stdio.h>

#include "sched.h"

#define REALTIME_PRIORITY 80

int go_realtime(void)
{
	int max_pri;
	struct sched_param sp;

	if (sched_getparam(0, &sp)) {
		perror("sched_getparam");
		return -1;
	}

	max_pri = sched_get_priority_max(SCHED_FIFO);
	sp.sched_priority = REALTIME_PRIORITY;

	if (sp.sched_priority > max_pri) {
		fprintf(stderr, "Invalid priority (maximum %d)\n", max_pri);
		return -1;
	}

	if (sched_setscheduler(0, SCHED_FIFO, &sp)) {
		perror("sched_setscheduler");
		return -1;
	}

	return 0;
}
