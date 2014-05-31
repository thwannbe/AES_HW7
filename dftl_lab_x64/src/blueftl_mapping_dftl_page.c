#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "blueftl_util.h"
#include "blueftl_ftl_base.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_mapping_page.h"

#include "blueftl_mapping_dftl_page.h"
#include "blueftl_gc_page.h"

//find the previous entry in the CMT
//static struct dftl_cached_mapping_entry_t *find_prev(struct dftl_context_t* dftl_context_t, struct dftl_cached_mapping_entry_t* target);

//insert the mapping into the CMT
static int insert_mapping(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context, uint32_t logical_page_address, uint32_t physical_page_address, struct dftl_cached_mapping_etnry_t* entry, int new);

//create the new cache entry 
static struct dftl_cached_mapping_entry_t* create_entry(uint32_t dirty, uint32_t logical_page_address, uint32_t physical_page_address);

//check the CMT whether it is full
static uint32_t isFull(struct dftl_context_t* ptr_dftl_context);

//evict the CMT entry
static void evict_cmt(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context);

//get a physical page address from the translation page by using the GTD
static uint32_t get_mapping_from_gtd(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context, uint32_t logical_page_address);

//write back the dirty translation page
static uint32_t write_back_tpage(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context, struct dftl_cached_mapping_entry_t* ptr_evict);

//translate the logical page address into the physical page address
uint32_t dftl_get_physical_address(
		struct ftl_context_t* ptr_ftl_context, 
		uint32_t logical_page_address){

	/*Write Your Own Code*/
	struct ftl_page_mapping_context_t* ptr_pg_mapping = 
		(struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;
	struct dftl_context_t* ptr_dftl_table = ptr_pg_mapping->ptr_dftl_table;
	struct dftl_cached_mapping_entry_t* dftl_cached_mapping_table_head = NULL;
	struct dftl_cached_mapping_entry_t* target_cached_mapping_entry = NULL;
	uint32_t physical_page_address;

	dftl_cached_mapping_table_head = ptr_dftl_table->ptr_cached_mapping_table_head;
	/* step 1. search in CMT ; hit -> end, miss -> next step */
	for(target_cached_mapping_entry=dftl_cached_mapping_table_head->next;
		target_cached_mapping_entry != dftl_cached_mapping_table_head;
		target_cached_mapping_entry = target_cached_mapping_entry->next) {
		if(target_cached_mapping_entry->logical_page_address == logical_page_address) {
			return target_cached_mapping_entry->physical_page_address; 
		}
	}

	/* step 2. if not in CMT, it should make new entry for CMT */
	/* before that we should read from GTD */
	if((physical_page_address = get_mapping_from_gtd(ptr_ftl_context, ptr_dftl_table, logical_page_address)) == -1) {
		printf("dftl_get_physical_address : No such page in GDT\n");
		return -2;
	}
	/* step 2-1. at first we should check CMT is full or not */
	if(isFull(ptr_dftl_table) == 1) {
		/* step 2-2. if full, we should evict one entry in CMT */
		evict_cmt(ptr_ftl_context, ptr_dftl_table);
	}
	/* step 2-2. find free slot in CMT, create new entry and allocate */
	if((target_cached_mapping_entry = create_entry(0, logical_page_address, physical_page_address)) == NULL) {
		return -1;
	}
	/* step 2-3. insert mapping */
	if((insert_mapping(ptr_ftl_context, ptr_dftl_table, logical_page_address, physical_page_address, target_cached_mapping_entry, 1)) == -1) {
		printf("dftl_get_physical_address : insert_mapping for new read entry is failed\n");
		return -1;
	}

	return physical_page_address;
}

//insert a new translation entry into the CMT 
uint32_t dftl_map_logical_to_physical(
		struct ftl_context_t* ptr_ftl_context,
		uint32_t logical_page_address,
		uint32_t physical_page_address){

	/*Write Your Own Code*/
	struct ftl_page_mapping_context_t* ptr_pg_mapping = 
		(struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;
	struct dftl_context_t* ptr_dftl_table = ptr_pg_mapping->ptr_dftl_table;
	struct dftl_cached_mapping_entry_t* dftl_cached_mapping_table_head = ptr_dftl_table->ptr_cached_mapping_table_head;
	struct dftl_cached_mapping_entry_t* target_cached_mapping_entry = NULL;
	int new;

	/* step 1. find target entry which match logical_page_address in CMT */
	for(target_cached_mapping_entry=dftl_cached_mapping_table_head->next;
		target_cached_mapping_entry != dftl_cached_mapping_table_head;
		target_cached_mapping_entry = target_cached_mapping_entry->next) {
		if(target_cached_mapping_entry->logical_page_address == logical_page_address) {
			new = 0; /* originally in CMT */
			goto find_matched;
		}
	}

	/* step 2. not find out matched entry -> new entry */
	if((target_cached_mapping_entry = create_entry(1, logical_page_address, physical_page_address)) == NULL) {
		printf("dftl_map_logical_to_physical : create_entry for new entry is failed\n");
		return -1;
	}
	new = 1; /* it is new entry */
	
find_matched:
	/* step 3. insert entry into CMT */
	if((insert_mapping(ptr_ftl_context, ptr_dftl_table, logical_page_address, physical_page_address, target_cached_mapping_entry, new)) == -1) {
		printf("dftl_map_logical_to_physical : insert_mapping for new read entry is failed\n");
		return -1;
	}

	return 0;
}

//modfiy the GTD
uint32_t dftl_modify_gtd(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context, uint32_t index, uint32_t physical_page_address){

	/*Write Your Own Code*/
	
}

static struct dftl_cached_mapping_entry_t* find_prev(struct dftl_context_t* dftl_context_t, struct dftl_cached_mapping_entry_t* target)
{
	struct dftl_cached_mapping_entry_t* ptr_cached_mapping_table_head =
		dftl_context_t->ptr_cached_mapping_table_head;
	struct dftl_cached_mapping_entry_t* loop = NULL;
	int cnt = 0; /* to avoid infinite loop */

	if(dftl_context_t->nr_cached_mapping_table_entries == 0 && target == ptr_cached_mapping_table_head) {
		return ptr_cached_mapping_table_head;
	}
	if(target == ptr_cached_mapping_table_head) {
		for(loop=ptr_cached_mapping_table_head->next; loop != target; loop = loop->next) {
			if(loop->next == target) {
				return loop;
			}
		}
	}
	else {
		for(loop=ptr_cached_mapping_table_head; loop != target && cnt < dftl_context_t->nr_cached_mapping_table_entries; loop = loop->next) {
			if(loop->next == target) {
				return loop;
			}
			cnt++;
		}
	}
	printf("find_prev : can't find target's previous entry in CMT\n");
	return NULL;
}

//insert the entry into the CMT
/*
static int insert_mapping(
		struct ftl_context_t* ptr_ftl_context,
		struct dftl_context_t* ptr_dftl_context,
		uint32_t logical_page_address,
		uint32_t physical_page_address,
		struct dftl_cached_mapping_etnry_t* entry,
		int new)*/
static int insert_mapping(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context, uint32_t logical_page_address, uint32_t physical_page_address, struct dftl_cached_mapping_etnry_t* entry, int new)
{
	
	/*Write Your Own Code*/
	struct dftl_cached_mapping_entry_t* dftl_cached_mapping_table_head = ptr_dftl_context->ptr_cached_mapping_table_head;
	struct dftl_cached_mapping_entry_t* prev = NULL;
	struct dftl_cached_mapping_entry_t* origin_last = NULL;

	/* step 1. check if cmt is full or not */
	if(isFull(ptr_dflt_context) == 1) {
		printf("insert_mapping : ptr_dftl_table is already full\n");
		return -1;
	}
	if(!new) { /* old entry reordered */
		if((prev = find_prev(ptr_dftl_context, entry)) == NULL) {
			printf("insert_mapping : can't find prev entry in CMT\n");
			return -1;
		}
		prev->next = entry->next;
	}
	entry->next = dftl_cached_mapping_table_head;
	if((origin_last = find_prev(ptr_dftl_context, dftl_cached_mapping_table_head)) == NULL) {
		printf("insert_mapping : can't find last entry in CMT\n");
		return -1;
	}
	origin_last->next = entry;

	ptr_dftl_table->nr_cached_mapping_table_entries++;

	return 0;
}

//create a new CMT entry
static struct dftl_cached_mapping_entry_t* create_entry(
		uint32_t dirty,
		uint32_t logical_page_address,
		uint32_t physical_page_address){
	
	/*Write Your Own Code*/
	struct dftl_cached_mapping_entry_t *tmp;
	if((tmp = (struct dftl_cached_mapping_entry_t*)malloc(sizeof(struct dftl_cached_mapping_entry_t))) == NULL) {
		printf("create_entry : memory allocation failed for new entry in CMT\n");
		return NULL;
	}
	tmp->logical_page_address = logical_page_address;
	tmp->physical_page_address = physical_page_address;
	tmp->dirty = dirty;

	return tmp;
}

//update the CMT by using eviction and inserting
uint32_t update_cmt(
		struct ftl_context_t* ptr_ftl_context, 
		struct dftl_context_t* ptr_dftl_context, 
		uint32_t logical_page_address, 
		uint32_t physical_page_address,
		int flag){

	/*Write Your Own Code*/

}

//check the CMT is full
static uint32_t isFull(
		struct dftl_context_t* ptr_dftl_context){

	/*Write Your Own Code*/
	return (ptr_dftl_context->nr_cached_mapping_table_entries == CMT_MAX) ? 1 : 0;

}

//evict the list by using LRU scheme
static void evict_cmt(
		struct ftl_context_t* ptr_ftl_context, 
		struct dftl_context_t* ptr_dftl_context){

	/*Write Your Own Code*/
	struct dftl_cached_mapping_entry_t* victim = ptr_dftl_context->ptr_cached_mapping_table_head->next;
	struct dftl_cached_mapping_entry_t* loop = NULL;
	uint32_t index, physical_tpage_address;

	if(victim->dirty == 0) { /* it could be evicted without gtd modification */
		ptr_dftl_context->ptr_cached_mapping_table_head->next = victim->next;
		free(victim);
		ptr_dftl_context->nr_cached_mapping_table_entries--;
	}
	else { /* it involves gtd modification */
		if((physical_tpage_address = write_back_tpage(ptr_ftl_context, ptr_dftl_context, victim)) == -1) {
			printf("evict_cmt : write back translation page is failed\n");
			exit(1);
		}
		index = victim->logical_page_address/512;
		if(dftl_modify_gtd(ptr_ftl_context, ptr_dftl_context, index, physical_tpage_address) == -1) {
			printf("evict_cmt : Global translation directory modification failed\n");
			exit(1);
		}
		/* batch eviction */
		for(loop = victim; loop != ptr_dftl_context->ptr_cached_mapping_table_head; ) {
			struct dftl_cached_mapping_entry_t* prev;
			if(loop->logical_page_address/512 == index) {
				prev = find_prev(ptr_dftl_context, loop);
				prev->next = loop->next;
				free(loop);
				ptr_dftl_context->nr_cached_mapping_table_entries--;
				loop = prev->next;
			}
		}
	}
	perf_dftl_cmt_eviction();
}

//get the physical page address from the translation page in the SSD by using the GTD
static uint32_t get_mapping_from_gtd(
		struct ftl_context_t* ptr_ftl_context,
		struct dftl_context_t* ptr_dftl_context,
		uint32_t logical_page_address){
	uint32_t physical_page_address, curr_bus, curr_chip, curr_block, curr_page, loop;
	/* curr_bus = curr_chip = 0 */
	uint32_t physical_translation_page_address, physical_translation_page_offset; 
	uint32_t* ptr_global_translation_directory = ptr_dftl_context->ptr_global_translation_directory;
	uint32_t ptr_page_in_gtd = logical_page_address/512;
	uint8_t* ptr_buff = (uint8_t*)malloc(sizeof(uint8_t) * FLASH_PAGE_SIZE);
	
	/*Write Your Own Code*/

	/* page_offset is aligned as uint8_t */
	physical_translation_page_offset = (logical_page_address % 512) * 4;
	if((physical_translation_page_address = ptr_global_translation_directory[ptr_page_in_gtd]) == GTD_FREE) {
		return -1;
	}
	ftl_convert_to_ssd_layout (physical_translation_page_address, &curr_bus, &curr_chip, &curr_block, &curr_page);
	/* now get target translation page ssd layout */
	
	blueftl_user_vdevice_page_read (
		_ptr_vdevice,
		curr_bus, curr_chip, curr_block, curr_page,
		sizeof(uint8_t) * FLASH_PAGE_SIZE,
		(char*)ptr_buff);
	perf_inc_tpage_reads();
	/* now ptr_buff has target translation page table */

	physical_page_address = 0;
	for(loop = 0; loop < 4; loop++)	{
		uint32_t tmp;
		physical_page_address = physical_page_address << 8;
		tmp = (uint32_t)ptr_buff[physical_translation_page_offset + 3 - loop];
		physical_page_address |= tmp;
	}
	/* now get real physical_page_address */
	
	if(ptr_buff)
		free(ptr_buff);
	
	return physical_page_address;
}

/* Write Back Dirty Translation Entry into Translation Page */
static uint32_t write_back_tpage(
		struct ftl_context_t* ptr_ftl_context,
		struct dftl_context_t* ptr_dftl_context,
		struct dftl_cached_mapping_entry_t* ptr_evict){

	/*Write Your Own Code*/

}

