#include "blueftl_ftl_base.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_user_vdevice.h"

/* for gc_type */
#define DBLOCK 0
#define TBLOCK 1

void print_reserved_block_status(struct ftl_page_mapping_context_t* ptr_pg_mapping);

int32_t shrink_translation_blocks (
	struct ftl_context_t* ptr_ftl_context,
	int32_t gc_target_bus,
	int32_t gc_target_chip);

struct flash_block_t* gc_dftl_select_victim_greedy (
	struct flash_ssd_t* ptr_ssd,
	int32_t gc_target_bus, 
	int32_t gc_target_chip);

int32_t gc_dftl_trigger_gc (
	struct ftl_context_t* ptr_ftl_context,
	int32_t gc_target_bus,
	int32_t gc_target_chip,
	int32_t gc_type);
