#ifndef __STATS_H__
#define __STATS_H__

#include <types.h>

struct tcp_stats {
    size_t tcp_discarded;
    size_t tcp_received;
    size_t tcp_dropped;
};

extern struct tcp_stats stats;

#endif
