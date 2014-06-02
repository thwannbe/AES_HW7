#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "blueftl_ftl_base.h"
#include "blueftl_mapping_page.h"
#include "blueftl_util.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_mapping_dftl_page.h"

unsigned char gc_buff[FLASH_PAGE_SIZE];

/* shrink the number of translation blocks until minimum number */
int32_t shrink_translation_blocks (
	struct ftl_context_t* ptr_ftl_context,
	int32_t gc_target_bus,
	int32_t gc_target_chip)
{
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	
	int32_t ret = 0;
	
	uint32_t loop_page = 0;
	uint32_t loop;

	struct ftl_page_mapping_context_t* ptr_pg_mapping =
		(struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;
	struct dftl_context_t* ptr_dftl_table = ptr_pg_mapping->ptr_dftl_table;

	unsigned char *entire_tpage_buff = (unsigned char*) malloc(2 * ptr_ssd->nr_pages_per_block * FLASH_PAGE_SIZE); /* entire translation page info */
	unsigned char *buff_stack = NULL;

	struct flash_block_t* new_tblock = NULL;

	/* it assume that all 128 translation page is allocated, and translation blocks are over than 2blocks */
	/* this function is focus on compressing into 2 valid translation blocks */

	/* step 1. trace all translation pages in translation blocks,
	 *	copy into buff, and erase all
	 */

	buff_stack = entire_tpage_buff;
	for(loop = 0; loop < 128; loop++) {
		uint32_t cur_tpage_paddr = ptr_dftl_table->ptr_global_translation_directory[loop];
		uint32_t tp_bus, tp_chip, tp_block, tp_page;
		struct flash_block_t* ptr_cur_tblock = NULL;
		struct flash_page_t* ptr_cur_tpage = NULL;
		if(cur_tpage_paddr == GTD_FREE) {
			printf("shrink_translation_blocks : GTD is not full\n");
			ret = -1;
			goto failed;
		}
		ftl_convert_to_ssd_layout(cur_tpage_paddr, &tp_bus, &tp_chip, &tp_block, &tp_page);
		ptr_cur_tblock = &(ptr_ssd->list_buses[tp_bus].list_chips[tp_chip].list_blocks[tp_block]);
		ptr_cur_tpage = &(ptr_ssd->list_buses[tp_bus].list_chips[tp_chip].list_blocks[tp_block].list_pages[tp_page]);
		if(ptr_cur_tpage->page_status != PAGE_STATUS_VALID) {
			printf("shrink_translation_blocks : current translation page[%u] should be valid (%u)\n", loop, ptr_cur_tpage->page_status);
			ret = -1;
			goto failed;
		}
		blueftl_user_vdevice_page_read(
			_ptr_vdevice,
			tp_bus, tp_chip, tp_block, tp_page,
			FLASH_PAGE_SIZE,
			(char*) buff_stack);
		perf_inc_tpage_reads();
		ptr_cur_tblock->last_modified_time = timer_get_timestamp_in_sec();

		if (ptr_cur_tblock->nr_valid_pages > 0) {
			ptr_cur_tblock->nr_invalid_pages++;
			ptr_cur_tblock->nr_valid_pages--;

			ptr_cur_tpage->page_status = PAGE_STATUS_INVALID;
		}
		else {
			printf("shrink_translation_blocks : nr_valid_pages is zero\n");
			ret = -1;
			goto failed;
		}
		buff_stack += FLASH_PAGE_SIZE;
	}
	/* now all translation info is in entire_tpage_buff */

	/* step 2. erase all translation block => the result should be guarantee at least 2 free blocks */
	for(loop = 0; loop < 128; loop++) {
		uint32_t cur_tpage_paddr = ptr_dftl_table->ptr_global_translation_directory[loop];
		uint32_t tp_bus, tp_chip, tp_block, tp_page;
		struct flash_block_t* ptr_cur_tblock = NULL;
		ftl_convert_to_ssd_layout(cur_tpage_paddr, &tp_bus, &tp_chip, &tp_block, &tp_page);
		ptr_cur_tblock = &(ptr_ssd->list_buses[tp_bus].list_chips[tp_chip].list_blocks[tp_block]);
		if(ptr_cur_tblock->nr_valid_pages != 0) {
			printf("shrink_translation_blocks : this translation block[%u] should be all invalid\n", tp_block);
			ret = -1;
			goto failed;
		}
		/* error checking */
		if((ptr_cur_tblock->nr_valid_pages + ptr_cur_tblock->nr_invalid_pages + ptr_cur_tblock->nr_free_pages) !=
			ptr_ssd->nr_pages_per_block) {
			printf("shrink_translation_blocks : this translation block[%u] has wrong nr block info\n", tp_block);
			ret = -1;
			goto failed;
		}
		/* if it is not free yet, make it free */
		if(ptr_cur_tblock->nr_invalid_pages == ptr_ssd->nr_pages_per_block) {
			blueftl_user_vdevice_block_erase(
				_ptr_vdevice,
				tp_bus, tp_chip, tp_block);
			perf_inc_tblock_erasures();

			ptr_cur_tblock->nr_free_pages = ptr_ssd->nr_pages_per_block;
			ptr_cur_tblock->nr_valid_pages = 0;
			ptr_cur_tblock->nr_invalid_pages = 0;
			ptr_cur_tblock->nr_erase_cnt++;
			ptr_cur_tblock->last_modified_time = 0;
		}

		for(loop_page = 0; loop_page < ptr_ssd->nr_pages_per_block; loop_page++) {
			ptr_cur_tblock->list_pages[loop_page].no_logical_page_addr = -1;
			ptr_cur_tblock->list_pages[loop_page].page_status = PAGE_STATUS_FREE;
		}
	}

	/* step 3. copy entire translation info in buffer into 2 free translation blocks */
	if(!(new_tblock = ssdmgmt_get_free_block (ptr_ssd, 0, 0))) {
		printf("shrink_translation_blocks : free block should exist\n");
		ret = -1;
		goto failed;
	}
	buff_stack = entire_tpage_buff; loop_page = 0;/* loop_page reuse */
	for(loop = 0; loop < 128; loop++) {
		uint32_t tp_bus, tp_chip, tp_block, tp_page;
		struct flash_page_t* ptr_cur_tpage = NULL;
		/* get new tblock's bus, chip, block, page */
		tp_bus = new_tblock->no_bus;
		tp_chip = new_tblock->no_chip;
		tp_block = new_tblock->no_block;
		tp_page = loop_page;
		ptr_cur_tpage = &(ptr_ssd->list_buses[tp_bus].list_chips[tp_chip].list_blocks[tp_block].list_pages[tp_page]);
		
		if(ptr_cur_tpage->page_status != PAGE_STATUS_FREE) {
			printf("shrink_translation_blocks : new translation page[%u] should be free (%u)\n", loop, ptr_cur_tpage->page_status);
			ret = -1;
			goto failed;
		}
		blueftl_user_vdevice_page_write(
			_ptr_vdevice,
			tp_bus, tp_chip, tp_block, tp_page,
			FLASH_PAGE_SIZE,
			(char*) buff_stack);
		perf_inc_tpage_writes();
		new_tblock->last_modified_time = timer_get_timestamp_in_sec();

		if (new_tblock->nr_free_pages > 0) {
			new_tblock->nr_valid_pages++;
			new_tblock->nr_free_pages--;

			ptr_cur_tpage->page_status = PAGE_STATUS_VALID;
		}
		else {
			printf("shrink_translation_blocks : nr_free_pages is zero\n");
			ret = -1;
			goto failed;
		}
		loop_page++;
		buff_stack += FLASH_PAGE_SIZE;
		ptr_dftl_table->ptr_global_translation_directory[loop] = ftl_convert_to_physical_page_address(tp_bus, tp_chip, tp_block, tp_page);
		if(loop_page > 63) { /* next new translation block is needed */
			if(!(new_tblock = ssdmgmt_get_free_block (ptr_ssd, 0, 0))) {
				printf("shrink_translation_blocks : free block should exist\n");
				ret = -1;
				goto failed;
			}
			loop_page = 0;
		}		
	}
	
failed:
	return ret;
	
}


//Victim selection for GC
struct flash_block_t* gc_dftl_select_victim_greedy (
	struct flash_ssd_t* ptr_ssd,
	int32_t gc_target_bus, 
	int32_t gc_target_chip)
{

	/* Write Your Own Code */
	struct flash_block_t* ptr_victim_block = NULL;

	uint32_t nr_max_invalid_pages = 0;
	uint32_t nr_cur_invalid_pages;
	uint32_t loop_block;

	for(loop_block = 0; loop_block < ptr_ssd->nr_blocks_per_chip; loop_block++) {
		nr_cur_invalid_pages = 
			ptr_ssd->list_buses[gc_target_bus].list_chips[gc_target_chip].list_blocks[loop_block].nr_invalid_pages;
		
		if(nr_cur_invalid_pages == ptr_ssd->nr_pages_per_block) {
			ptr_victim_block =
				&(ptr_ssd->list_buses[gc_target_bus].list_chips[gc_target_chip].list_blocks[loop_block]);
			return ptr_victim_block;
		}

		if(nr_max_invalid_pages < nr_cur_invalid_pages)
		{
			nr_max_invalid_pages = nr_cur_invalid_pages;
			ptr_victim_block =
				&(ptr_ssd->list_buses[gc_target_bus].list_chips[gc_target_chip].list_blocks[loop_block]);
		}
	}

	if (ptr_victim_block == NULL) {
		printf("gc_dftl_select_victim_greedy : 'ptr_victim_block' is NULL\n");
	}
	
	return ptr_victim_block;
}

//Garbage Collection for DFTL gc_type for data page GC or translation page GC 
int32_t gc_dftl_trigger_gc (
	struct ftl_context_t* ptr_ftl_context,
	int32_t gc_target_bus,
	int32_t gc_target_chip,
	int32_t gc_type /* DBLOCK = 0, TBLOCK = 1 */
	)
{

	/* Write Your Own Code */
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	
	struct flash_block_t* ptr_gc_block = NULL;
	struct flash_block_t* ptr_victim_block = NULL;
	
	int32_t victim_type = 0;	/* DBLOCK = 0, TBLOCK = 1 */
	int32_t ret = 0;
	
	uint32_t loop_page = 0;
	uint32_t loop_page_victim = 0;
	uint32_t loop_page_gc = 0;
	uint32_t logical_page_address;
	uint32_t loop;

	struct ftl_page_mapping_context_t* ptr_pg_mapping =
		(struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;
	struct dftl_context_t* ptr_dftl_table = ptr_pg_mapping->ptr_dftl_table;
	
	/* step 1. select victim_block */
	if((ptr_victim_block = gc_dftl_select_victim_greedy(ptr_ssd, gc_target_bus, gc_target_chip)) == NULL) {
		printf("gc_dftl_trigger_gc : select victim block is failed\n");
		ret = -1;
		goto failed;
	}

	if(ptr_victim_block->nr_invalid_pages == 0){
		printf("gc_dftl_trigger_gc : the victim block has at least 1 invalid page\n");
		ret = -1;
		goto failed;
	}
	
	/* now we need to know victim block is tblock or dblock */
	for(loop = 0; loop < 128; loop++) {
		uint32_t curr_bus, curr_chip, curr_block, curr_page;
		if(ptr_dftl_table->ptr_global_translation_directory[loop] != GTD_FREE) {
			ftl_convert_to_ssd_layout(ptr_dftl_table->ptr_global_translation_directory[loop], &curr_bus, &curr_chip, &curr_block, &curr_page);
			if(ptr_victim_block->no_bus == curr_bus && ptr_victim_block->no_chip == curr_chip && ptr_victim_block->no_block == curr_block) {
				victim_type = 1; /* TBLOCK */
				break;
			}
		}
	}

	/* step 2. prepare gc reserved block */
	if(victim_type) {
		if((ptr_gc_block = *(ptr_pg_mapping->ptr_gc_blocks)) == NULL) {
			printf("gc_dftl_trigger_gc : there is no reserved gc block\n");
			ret = -1;
			goto failed;
		}
	}
	else { /* victim block = DBLOCK */
		if((ptr_gc_block = *(ptr_pg_mapping->ptr_gc_tblocks)) == NULL) {
			printf("gc_dftl_trigger_gc : there is no reserved gc tblock\n");
			ret = -1;
			goto failed;
		}
	}
	/* check error cases */
	if(ptr_gc_block->nr_free_pages != ptr_ssd->nr_pages_per_block || ptr_gc_block->is_reserved_block != 1) {
		printf("gc_dftl_trigger_gc : gc block is not appropriate\n");
		ret = -1;
		goto failed;
	}
	/* now gc reserved block is ready */

	/* step 3. copy flash pages from the victim block to the gc block */
	if(!victim_type) { /* DBLOCK */
		if(ptr_victim_block->nr_valid_pages >0){
			for(loop_page_victim = 0; loop_page_victim < ptr_ssd->nr_pages_per_block; loop_page_victim++) {
				struct flash_page_t* ptr_cur_page = &(ptr_victim_block->list_pages[loop_page_victim]);
				if(ptr_cur_page->page_status == PAGE_STATUS_VALID) {
					logical_page_address = ptr_cur_page->no_logical_page_addr;
					blueftl_user_vdevice_page_read(
							_ptr_vdevice,
							ptr_victim_block->no_bus,
							ptr_victim_block->no_chip,
							ptr_victim_block->no_block,
							loop_page_victim,
							FLASH_PAGE_SIZE,
							(char*) gc_buff);

					blueftl_user_vdevice_page_write(
							_ptr_vdevice,
							ptr_gc_block->no_bus,
							ptr_gc_block->no_chip,
							ptr_gc_block->no_block,
							loop_page_gc,
							FLASH_PAGE_SIZE,
							(char*) gc_buff);

					if(dftl_mapping_map_logical_to_physical(
						ptr_ftl_context,
						logical_page_address,
						ptr_gc_block->no_bus,
						ptr_gc_block->no_chip,
						ptr_gc_block->no_block,
						loop_page_gc, 0) == -1)
					{
						/* one more chance */
						if(shrink_translation_blocks(ptr_ftl_context, 0, 0) == -1) {
							printf("gc_dftl_trigger_gc : shrink tblock is failed\n");
							ret = -1;
							goto failed;
						}
						if(dftl_mapping_map_logical_to_physical(
							ptr_ftl_context,
							logical_page_address,
							ptr_gc_block->no_bus,
							ptr_gc_block->no_chip,
							ptr_gc_block->no_block,
							loop_page_gc, 0) == -1) {

							printf("gc_dftl_trigger_gc : victim block's mapping log to phy is failed\n");
							ret = -1;
							goto failed;
						}
					}
					
					/* copy success check */
					if(ptr_cur_page->page_status != PAGE_STATUS_INVALID)
					{
						printf("gc_dftl_trigger_gc : the victim page is not invalidated\n");
						ret = -1;
						goto failed;
					}
					if(ptr_gc_block->list_pages[loop_page_gc].page_status != PAGE_STATUS_VALID)
					{
						printf("gc_dftl_trigger_gc : the gc page is not valid\n");
						ret = -1;
						goto failed;
					}

					loop_page_gc++;
					perf_gc_inc_page_copies ();
				}
			}
		}
	}
	else { /* TBLOCK */
		if(ptr_victim_block->nr_valid_pages >0){
			for(loop_page_victim = 0; loop_page_victim < ptr_ssd->nr_pages_per_block; loop_page_victim++) {
				struct flash_page_t* ptr_cur_page = &(ptr_victim_block->list_pages[loop_page_victim]);
				if(ptr_cur_page->page_status == PAGE_STATUS_VALID) {
					logical_page_address = ptr_cur_page->no_logical_page_addr;
					blueftl_user_vdevice_page_read(
							_ptr_vdevice,
							ptr_victim_block->no_bus,
							ptr_victim_block->no_chip,
							ptr_victim_block->no_block,
							loop_page_victim,
							FLASH_PAGE_SIZE,
							(char*) gc_buff);

					blueftl_user_vdevice_page_write(
							_ptr_vdevice,
							ptr_gc_block->no_bus,
							ptr_gc_block->no_chip,
							ptr_gc_block->no_block,
							loop_page_gc,
							FLASH_PAGE_SIZE,
							(char*) gc_buff);

					/* 'loop' is GTD index for modification */
					ptr_dftl_table->ptr_global_translation_directory[loop] = 
						ftl_convert_to_physical_page_address(
							ptr_gc_block->no_bus,
							ptr_gc_block->no_chip,
							ptr_gc_block->no_block,
							loop_page_gc);
					
					ptr_victim_block->last_modified_time = timer_get_timestamp_in_sec();

					if (ptr_victim_block->nr_valid_pages > 0) {
						ptr_victim_block->nr_invalid_pages++;
						ptr_victim_block->nr_valid_pages--;

						ptr_cur_page->page_status = PAGE_STATUS_INVALID;
					}
					else {
						printf("gc_dftl_trigger_gc : nr_valid_pages is zero\n");
						ret = -1;
						goto failed;
					}
					
					if ((ptr_victim_block->nr_free_pages + ptr_victim_block->nr_valid_pages + ptr_victim_block->nr_invalid_pages) != ptr_ssd->nr_pages_per_block) {
						printf("gc_dftl_trigger_gc : free:%u + valid:%u + invalid:%u is not %u\n",
							ptr_victim_block->nr_free_pages, ptr_victim_block->nr_valid_pages,
							ptr_victim_block->nr_invalid_pages, ptr_ssd->nr_pages_per_block);
						ret = -1;
						goto failed;
					}

					if(ptr_gc_block->nr_free_pages > 0) {
						ptr_gc_block->nr_valid_pages++;
						ptr_gc_block->nr_free_pages--;

						if (ptr_gc_block->list_pages[loop_page_gc].page_status != PAGE_STATUS_FREE) {
							printf("gc_dftl_trigger_gc : the given page should be free (%u) (%u)\n",
								ptr_gc_block->list_pages[loop_page_gc].page_status, loop_page_gc);
							ret = -1;
							goto failed;
						}
					}
					else {
						printf("gc_dftl_trigger_gc : ptr gc block's nr_free_pages is zero before writting\n");
						ret = -1;
						goto failed;
					}
					
					if ((ptr_gc_block->nr_free_pages + ptr_gc_block->nr_valid_pages + ptr_gc_block->nr_invalid_pages) != ptr_ssd->nr_pages_per_block) {
						printf("gc_dftl_trigger_gc : free:%u + valid:%u + invalid:%u is not %u\n",
							ptr_gc_block->nr_free_pages, ptr_gc_block->nr_valid_pages,
							ptr_gc_block->nr_invalid_pages, ptr_ssd->nr_pages_per_block);
						ret = -1;
						goto failed;
					}
				}
			}
		}
	}

	blueftl_user_vdevice_block_erase(
			_ptr_vdevice,
			ptr_victim_block->no_bus,
			ptr_victim_block->no_chip,
			ptr_victim_block->no_block);

	if(victim_type) /* TBLOCK */
		perf_gc_inc_tblk_erasures();
	else
		perf_gc_inc_blk_erasures();

	ptr_victim_block->nr_free_pages = ptr_ssd->nr_pages_per_block;
	ptr_victim_block->nr_valid_pages = 0;
	ptr_victim_block->nr_invalid_pages = 0;
	ptr_victim_block->nr_erase_cnt++;
	ptr_victim_block->last_modified_time = 0;
	ptr_victim_block->is_reserved_block = 1; /* it becomes new gc reserved block */

	for(loop_page = 0; loop_page < ptr_ssd->nr_pages_per_block; loop_page++) {
		ptr_victim_block->list_pages[loop_page].no_logical_page_addr = -1;
		ptr_victim_block->list_pages[loop_page].page_status = PAGE_STATUS_FREE;
	}

	if(victim_type) /* victim block was TBLOCK */
		*(ptr_pg_mapping->ptr_gc_tblocks) = ptr_victim_block;
	else
		*(ptr_pg_mapping->ptr_gc_blocks) = ptr_victim_block;

	ptr_gc_block->is_reserved_block = 0;

	if(gc_type) /* gc for new TBLOCK */
		*(ptr_pg_mapping->ptr_translation_blocks) = ptr_gc_block; /* now new translation block for TBLOCK is ptr_gc_block */
	else
		*(ptr_pg_mapping->ptr_active_blocks) = ptr_gc_block; /* now new active block for DBLOCK is ptr_gc_block */

failed:
	return ret;
}
