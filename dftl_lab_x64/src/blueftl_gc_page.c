#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "blueftl_ftl_base.h"
#include "blueftl_mapping_page.h"
#include "blueftl_util.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_mapping_dftl_page.h"

unsigned char gc_buff[FLASH_PAGE_SIZE];
extern int 	  _param_wl_policy;

//Victim selection for GC
struct flash_block_t* gc_dftl_select_victim_greedy (
	struct flash_ssd_t* ptr_ssd,
	int32_t gc_target_bus, 
	int32_t gc_target_chip,
	int32_t gc_type,
	struct ftl_page_mapping_context_t* ptr_pg_mapping)
{

	/* Write Your Own Code */

	return ptr_victim_block;
}

//Garbage Collection for DFTL gc_type for data page GC or translation page GC 
int32_t gc_dftl_trigger_gc (
	struct ftl_context_t* ptr_ftl_context,
	int32_t gc_target_bus,
	int32_t gc_target_chip,
	int32_t gc_type
	)
{

	/* Write Your Own Code */

	return ret;
}
