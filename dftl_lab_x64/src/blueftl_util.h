#ifndef _BLUESSD_UTIL
#define _BLUESSD_UTIL

#include <fcntl.h>

#if !defined( min )
# define min(a,b) (a<b?a:b)
#endif

#if !defined( max )
# define max(a,b) (a>b?a:b)
#endif

#define MAX_SUMMARY_BUFFER 4096

/* get current time in microsecond */
uint32_t timer_get_timestamp_in_us (void);
uint32_t timer_get_timestamp_in_sec (void);
void timer_init (void);

/* performance profiling */
void perf_init (void);
//For Page Read
void perf_inc_page_reads (void);
//For Page Write 
void perf_inc_page_writes (void);
//For Block Erasure 
void perf_inc_blk_erasures (void);
//For Page Copies in GC 
void perf_gc_inc_page_copies (void);
//For Block Erasures in GC 
void perf_gc_inc_blk_erasures (void);
void perf_display_results (void);
/* for DFTL */
//For CMT Eviction 
void perf_dftl_cmt_eviction(void);
//For Translation Page Read
void perf_inc_tpage_reads (void);
//For Translation Page Write 
void perf_inc_tpage_writes (void);
//For Translation Block Erasure 
void perf_inc_tblock_erasures (void);
//For Translation Block Copies in GC 
void perf_gc_inc_tpage_copies (void);
//For Translation Block Erasure in GC 
void perf_gc_inc_tblk_erasures (void);

#endif
