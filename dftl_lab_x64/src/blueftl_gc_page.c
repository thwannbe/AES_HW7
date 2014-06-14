#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "blueftl_ftl_base.h"
#include "blueftl_mapping_page.h"
#include "blueftl_util.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_mapping_dftl_page.h"

unsigned char gc_buff[FLASH_PAGE_SIZE];

void print_reserved_block_status(struct ftl_page_mapping_context_t* ptr_pg_mapping) {
	struct flash_block_t *target = NULL;
	printf("****************** RESERVED BLOCK STATUS ********************\n");
	printf("[ ptr_translation_blocks ]\n");
	if((target = ptr_pg_mapping->ptr_translation_blocks[0]) != NULL) {
		printf("- bus [%u] chip [%u] block[%u]\n", target->no_bus, target->no_chip, target->no_block);
		printf("- free [%u] - valid [%u] - invalid [%u] - total [%u/%u]\n",
			target->nr_free_pages, target->nr_valid_pages, target->nr_invalid_pages,
			target->nr_free_pages + target->nr_valid_pages + target->nr_invalid_pages,
			NR_PAGES_PER_BLOCK);
		printf("- is_reserved_block [%u]\n", target->is_reserved_block);
	} else {
		printf("- NULL BLOCK\n");
	}

	printf("[ ptr_active_blocks ]\n");
	if((target = ptr_pg_mapping->ptr_active_blocks[0]) != NULL) {
		printf("- bus [%u] chip [%u] block[%u]\n", target->no_bus, target->no_chip, target->no_block);
		printf("- free [%u] - valid [%u] - invalid [%u] - total [%u/%u]\n",
			target->nr_free_pages, target->nr_valid_pages, target->nr_invalid_pages,
			target->nr_free_pages + target->nr_valid_pages + target->nr_invalid_pages,
			NR_PAGES_PER_BLOCK);
		printf("- is_reserved_block [%u]\n", target->is_reserved_block);
	} else {
		printf("- NULL BLOCK\n");
	}

	printf("[ ptr_gc_tblocks ]\n");
	if((target = ptr_pg_mapping->ptr_gc_tblocks[0]) != NULL) {
		printf("- bus [%u] chip [%u] block[%u]\n", target->no_bus, target->no_chip, target->no_block);
		printf("- free [%u] - valid [%u] - invalid [%u] - total [%u/%u]\n",
			target->nr_free_pages, target->nr_valid_pages, target->nr_invalid_pages,
			target->nr_free_pages + target->nr_valid_pages + target->nr_invalid_pages,
			NR_PAGES_PER_BLOCK);
		printf("- is_reserved_block [%u]\n", target->is_reserved_block); 
	} else {
		printf("- NULL BLOCK\n");
	}

	printf("[ ptr_gc_blocks ]\n");
	if((target = ptr_pg_mapping->ptr_gc_blocks[0]) != NULL) {
		printf("- bus [%u] chip [%u] block[%u]\n", target->no_bus, target->no_chip, target->no_block);
		printf("- free [%u] - valid [%u] - invalid [%u] - total [%u/%u]\n",
			target->nr_free_pages, target->nr_valid_pages, target->nr_invalid_pages,
			target->nr_free_pages + target->nr_valid_pages + target->nr_invalid_pages,
			NR_PAGES_PER_BLOCK);
		printf("- is_reserved_block [%u]\n", target->is_reserved_block); 
	} else {
		printf("- NULL BLOCK\n");
	}
	printf("*************************************************************\n");
}

//Victim selection for GC
struct flash_block_t* gc_dftl_select_victim_greedy (
	struct dftl_context_t* ptr_dftl_table, 
	struct flash_ssd_t* ptr_ssd,
	int32_t gc_target_bus, 
	int32_t gc_target_chip,
	int32_t gc_type) // gc_type [0] : DBLOCK, [1] : TBLOCK
{

	/* Write Your Own Code */
	struct flash_block_t* ptr_victim_block = NULL;

	uint32_t nr_max_invalid_pages = 0;
	uint32_t nr_cur_invalid_pages;
	uint32_t loop_block;
	uint32_t loop;
	
	if(!gc_type) { // gc type = DBLOCK => victim block is DBLOCK or TBLOCK
		for(loop_block = 0; loop_block < ptr_ssd->nr_blocks_per_chip; loop_block++) {
			nr_cur_invalid_pages = 
				ptr_ssd->list_buses[gc_target_bus].list_chips[gc_target_chip].list_blocks[loop_block].nr_invalid_pages;
			if(ptr_ssd->list_buses[gc_target_bus].list_chips[gc_target_chip].list_blocks[loop_block].is_reserved_block == 0) {	
				if(nr_cur_invalid_pages == NR_PAGES_PER_BLOCK) {
					ptr_victim_block =
						&(ptr_ssd->list_buses[gc_target_bus].list_chips[gc_target_chip].list_blocks[loop_block]);
					return ptr_victim_block;
				}

				if(nr_max_invalid_pages < nr_cur_invalid_pages) {
					nr_max_invalid_pages = nr_cur_invalid_pages;
					ptr_victim_block =
						&(ptr_ssd->list_buses[gc_target_bus].list_chips[gc_target_chip].list_blocks[loop_block]);
				}
			}
		}
	} else { // gc type = TBLOCK => victim block is always TBLOCK
		for(loop = 0; loop < 128; loop++) {
			uint32_t curr_bus, curr_chip, curr_block, curr_page;
			struct flash_block_t* ptr_curr_block = NULL;
			if(ptr_dftl_table->ptr_global_translation_directory[loop] != GTD_FREE) {
				ftl_convert_to_ssd_layout(ptr_dftl_table->ptr_global_translation_directory[loop], &curr_bus, &curr_chip, &curr_block, &curr_page);
				ptr_curr_block = &ptr_ssd->list_buses[curr_bus].list_chips[curr_chip].list_blocks[curr_block];
				if(nr_max_invalid_pages < ptr_curr_block->nr_invalid_pages && ptr_curr_block->is_reserved_block == 0) {
					ptr_victim_block = ptr_curr_block;
					nr_max_invalid_pages = ptr_curr_block->nr_invalid_pages;
				}
			}
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
	struct dftl_cached_mapping_entry_t* dftl_cached_mapping_table_head = ptr_dftl_table->ptr_cached_mapping_table_head;

	/* step 1. select victim_block */
	if((ptr_victim_block = gc_dftl_select_victim_greedy(ptr_dftl_table, ptr_ssd, gc_target_bus, gc_target_chip, gc_type)) == NULL) {
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
	if(!victim_type) {
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
	if(ptr_gc_block->nr_free_pages != NR_PAGES_PER_BLOCK || ptr_gc_block->is_reserved_block != 1) {
		printf("gc_dftl_trigger_gc : gc block is not appropriate\n");
		print_reserved_block_status(ptr_pg_mapping);
		if(ptr_gc_block->nr_free_pages != NR_PAGES_PER_BLOCK) {
			for(loop_page = 0; loop_page < NR_PAGES_PER_BLOCK; loop_page++) {
				if(ptr_gc_block->list_pages[loop_page].page_status == PAGE_STATUS_VALID) {
					printf("gc_dftl_trigger_gc : gc block's valid page[%u] ; LPA [%u]\n", loop_page, ptr_gc_block->list_pages[loop_page].no_logical_page_addr);
				}
			}
		}
		ret = -1;
		goto failed;
	}
	/* now gc reserved block is ready */

	/* step 3. copy flash pages from the victim block to the gc block */
	if(!victim_type) { /* DBLOCK */
		if(ptr_victim_block->nr_valid_pages >0){
			for(loop_page_victim = 0; loop_page_victim < NR_PAGES_PER_BLOCK; loop_page_victim++) {
				struct flash_page_t* ptr_cur_page = &(ptr_victim_block->list_pages[loop_page_victim]);
				struct flash_block_t* translation_block = NULL;
				uint32_t* ptr_global_translation_directory = ptr_dftl_table->ptr_global_translation_directory;
				uint32_t index_global_translation_directory;
				uint32_t physical_translation_page_address;
				uint32_t physical_translation_page_offset;
				struct dftl_cached_mapping_entry_t* target_cached_mapping_entry = NULL;
				uint32_t modified_physical_page_address;

				uint32_t bus, chip, block, page;
				uint8_t* block_buff = (uint8_t*)malloc(sizeof(uint8_t) * FLASH_PAGE_SIZE * NR_PAGES_PER_BLOCK);
				uint8_t* page_pointer = 0; 
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

					ptr_victim_block->last_modified_time = timer_get_timestamp_in_sec ();
					ptr_victim_block->nr_invalid_pages++;
					ptr_victim_block->nr_valid_pages--;
					ptr_cur_page->page_status = PAGE_STATUS_INVALID;
					ptr_cur_page->no_logical_page_addr = -1;

					ptr_gc_block->last_modified_time = timer_get_timestamp_in_sec ();
					ptr_gc_block->nr_valid_pages++;
					ptr_gc_block->nr_free_pages--;
					ptr_gc_block->list_pages[loop_page_gc].page_status = PAGE_STATUS_VALID;
					ptr_gc_block->list_pages[loop_page_gc].no_logical_page_addr = logical_page_address;
				
					modified_physical_page_address = ftl_convert_to_physical_page_address(ptr_gc_block->no_bus, ptr_gc_block->no_chip, ptr_gc_block->no_block, loop_page_gc);
					for(target_cached_mapping_entry=dftl_cached_mapping_table_head->next;
							target_cached_mapping_entry != dftl_cached_mapping_table_head;
							target_cached_mapping_entry = target_cached_mapping_entry->next) {
						if(target_cached_mapping_entry->logical_page_address == logical_page_address) {
							target_cached_mapping_entry->physical_page_address = modified_physical_page_address;
							insert_mapping(ptr_ftl_context, ptr_dftl_table, target_cached_mapping_entry, 0);
							goto check_out;
						}
					}
					/* in this case we should update translation page and GTD */
					/* step 1. get translation physical_page_address */
					index_global_translation_directory = logical_page_address/512;
					if((physical_translation_page_address = ptr_global_translation_directory[index_global_translation_directory]) == GTD_FREE) {
						printf("gc_dftl_trigger_gc : index_global_translation_directory is GTD_FREE\n");
						ret = -1;
						goto failed;
					}
					ftl_convert_to_ssd_layout(physical_translation_page_address, &bus, &chip, &block, &page);
					translation_block = &ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block];
					/* step 2. copy translation block into buffer */
					page_pointer = block_buff;
					for(loop_page = 0; loop_page < NR_PAGES_PER_BLOCK; loop_page++) {
						blueftl_user_vdevice_page_read (
								_ptr_vdevice,
								bus, chip, block, loop_page,
								sizeof(uint8_t) * FLASH_PAGE_SIZE,
								(char*)page_pointer);
						perf_inc_tpage_reads();
						page_pointer += FLASH_PAGE_SIZE;
					}
					/* step 3. modify translation page data in buffer as change */
					physical_translation_page_offset = (logical_page_address % 512) * 4;
					page_pointer = block_buff + page * FLASH_PAGE_SIZE; //starting point of translation page
					for(loop = 0; loop < 4; loop++) {
						uint8_t tmp;
						uint32_t tmp_32;
						tmp_32 = (modified_physical_page_address >> 8*(3-loop)) & 0xff; /*  taken only 1 byte */
						tmp = (uint8_t) tmp_32;
						page_pointer[physical_translation_page_offset + 3 - loop] = tmp;
					}
					/* step 4. erase translation block */
					blueftl_user_vdevice_block_erase(
							_ptr_vdevice,
							bus, chip, block);
					perf_gc_inc_tblk_erasures();
					translation_block->nr_erase_cnt++;
					translation_block->last_modified_time = timer_get_timestamp_in_sec();
					// nothing change, just erase and upload modified data.
					/* step 5. copy modified translation data into erased block */
					page_pointer = block_buff;
					for(loop_page = 0; loop_page < NR_PAGES_PER_BLOCK; loop_page++) {
						blueftl_user_vdevice_page_write (
								_ptr_vdevice,
								bus, chip, block, loop_page,
								sizeof(uint8_t) * FLASH_PAGE_SIZE,
								(char*)page_pointer);
						perf_inc_tpage_writes();
						page_pointer += FLASH_PAGE_SIZE;
					}

check_out:
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
			for(loop_page_victim = 0; loop_page_victim < NR_PAGES_PER_BLOCK; loop_page_victim++) {
				struct flash_page_t* ptr_cur_page = &(ptr_victim_block->list_pages[loop_page_victim]);
				if(ptr_cur_page->page_status == PAGE_STATUS_VALID) {
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
				
					/* update GTD */
					/* step 1. find out current translation entry in GTD */
					for(loop = 0; loop < 128; loop++) {
						uint32_t curr_bus, curr_chip, curr_block, curr_page;
						if(ptr_dftl_table->ptr_global_translation_directory[loop] != GTD_FREE) {
							ftl_convert_to_ssd_layout(ptr_dftl_table->ptr_global_translation_directory[loop], &curr_bus, &curr_chip, &curr_block, &curr_page);
							if(ptr_victim_block->no_bus == curr_bus && ptr_victim_block->no_chip == curr_chip && ptr_victim_block->no_block == curr_block && loop_page_victim == curr_page) {
								goto find_out; // now 'loop' is current entry index in GTD
							}
						}
					}
					// can't find out -> error
					printf("gc_dftl_trigger_gc : current TPAGE is not correct\n");
					ret = -1;
					goto failed;
					
find_out:
					/* step 2. modify PPA in GTD */
					ptr_dftl_table->ptr_global_translation_directory[loop] = ftl_convert_to_physical_page_address(ptr_gc_block->no_bus, ptr_gc_block->no_chip, ptr_gc_block->no_block, loop_page_gc);
					
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
					
					if ((ptr_victim_block->nr_free_pages + ptr_victim_block->nr_valid_pages + ptr_victim_block->nr_invalid_pages) != NR_PAGES_PER_BLOCK) {
						printf("gc_dftl_trigger_gc : free:%u + valid:%u + invalid:%u is not %u\n",
							ptr_victim_block->nr_free_pages, ptr_victim_block->nr_valid_pages,
							ptr_victim_block->nr_invalid_pages, NR_PAGES_PER_BLOCK);
						ret = -1;
						goto failed;
					}

					if(ptr_gc_block->nr_free_pages > 0) {
						ptr_gc_block->nr_valid_pages++;
						ptr_gc_block->nr_free_pages--;
						ptr_gc_block->list_pages[loop_page_gc].page_status = PAGE_STATUS_VALID;
						/*
						if (ptr_gc_block->list_pages[loop_page_gc].page_status != PAGE_STATUS_FREE) {
							printf("gc_dftl_trigger_gc : the given page should be free (%u) (%u)\n",
									ptr_gc_block->list_pages[loop_page_gc].page_status, loop_page_gc);
							print_reserved_block_status(ptr_pg_mapping);
							for(loop = 0; loop <ptr_ssd->nr_pages_per_block; loop++) {
								printf("page[%u] => status[%u]\n", loop, ptr_gc_block->list_pages[loop].page_status);
							}
							ret = -1;
							goto failed;
						}*/
					}
					else {
						printf("gc_dftl_trigger_gc : ptr gc block's nr_free_pages is zero before writting\n");
						ret = -1;
						goto failed;
					}
					
					if ((ptr_gc_block->nr_free_pages + ptr_gc_block->nr_valid_pages + ptr_gc_block->nr_invalid_pages) != NR_PAGES_PER_BLOCK) {
						printf("gc_dftl_trigger_gc : free:%u + valid:%u + invalid:%u is not %u\n",
							ptr_gc_block->nr_free_pages, ptr_gc_block->nr_valid_pages,
							ptr_gc_block->nr_invalid_pages, NR_PAGES_PER_BLOCK);
						ret = -1;
						goto failed;
					}

					loop_page_gc++;
					perf_gc_inc_tpage_copies ();
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

	ptr_victim_block->nr_free_pages = NR_PAGES_PER_BLOCK;
	ptr_victim_block->nr_valid_pages = 0;
	ptr_victim_block->nr_invalid_pages = 0;
	ptr_victim_block->nr_erase_cnt++;
	ptr_victim_block->last_modified_time = 0;
	ptr_victim_block->is_reserved_block = 1; /* it becomes new gc reserved block */

	for(loop_page = 0; loop_page < NR_PAGES_PER_BLOCK; loop_page++) {
		ptr_victim_block->list_pages[loop_page].no_logical_page_addr = -1;
		ptr_victim_block->list_pages[loop_page].page_status = PAGE_STATUS_FREE;
	}

	if(victim_type) /* victim block was TBLOCK */
		*(ptr_pg_mapping->ptr_gc_tblocks) = ptr_victim_block;
	else
		*(ptr_pg_mapping->ptr_gc_blocks) = ptr_victim_block;

	ptr_gc_block->is_reserved_block = 1;

	if(gc_type) /* gc for new TBLOCK */
		*(ptr_pg_mapping->ptr_translation_blocks) = ptr_gc_block; /* now new translation block for TBLOCK is ptr_gc_block */
	else
		*(ptr_pg_mapping->ptr_active_blocks) = ptr_gc_block; /* now new active block for DBLOCK is ptr_gc_block */

failed:
	return ret;
}
