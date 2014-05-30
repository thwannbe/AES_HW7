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

//insert the mapping into the CMT
static int insert_mapping(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context, uint32_t logical_page_address, uint32_t physical_page_address, int flag);

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
	struct dftl_cached_mapping_entry_t** dftl_cached_mapping_table = ptr_dftl_table->ptr_cached_mapping_table;
	struct dftl_cached_mapping_entry_t* new_cached_mapping_entry = NULL;
	uint32_t i, nr_entries, physical_page_address;

	/* step 1. search in CMT ; hit -> end, miss -> next step */
	nr_entries = ptr_dftl_table->nr_cached_mapping_table_entries;
	for(i=0; i<nr_entries; i++) {
		if(dftl_cached_mapping_table[i]->logical_page_address == logical_page_address) {
			return dftl_cached_mapping_table[i]->physical_page_address; 
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
	/* step 2-3. search just evicted entry(from step 2-2) or fist free entry(from 2-1) in CMT */
	for(i=0; i<nr_entries; i++) {
		if(dftl_cached_mapping_table[i] == NULL) {
			goto find_evicted;
		}
	}
	/* error : can't find evicted entry */
	printf("dftl_get_physical_address : evict_cmt failed, there is no free entry in CMT\n");
	return -1;

find_evicted:
	/* step 2-4. find free slot in CMT, create new entry and allocate */
	if((new_cached_mapping_entry = create_entry(0, logical_page_address, physical_page_address)) == NULL) {
		return -1;
	}
	dftl_cached_mapping_table[i] = new_cached_mapping_entry;
	ptr_dftl_table->nr_cached_mapping_table_entries++;

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
	struct dftl_cached_mapping_entry_t** dftl_cached_mapping_table = ptr_dftl_table->ptr_cached_mapping_table;
	struct dftl_cached_mapping_entry_t* target_page_entry = NULL;
	uint32_t i, nr_entries;

	/* step 1. find target entry which match logical_page_address in CMT */
	nr_entries = ptr_dftl_table->nr_cached_mapping_table_entries;
	for(i=0; i<nr_entries; i++) {
		if(dftl_cached_mapping_table[i]->logical_page_address == logical_page_address) {
			goto find_matched;
		}
	}
	/* not find out matched entry -> error */
	printf("dftl_map_logical_to_physical : No such page in CMT\n");
	return -1;

find_matched:
	target_page_entry = dftl_cached_mapping_table[i];
	target_page_entry->physical_page_address = physical_page_address;
	target_page_entry->dirty = 1;

	return 0;
}

//modfiy the GTD
uint32_t dftl_modify_gtd(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context, uint32_t index, uint32_t physical_page_address){

	/*Write Your Own Code*/
	
}

//insert the entry into the CMT
static int insert_mapping(
		struct ftl_context_t* ptr_ftl_context,
		struct dftl_context_t* ptr_dftl_context,
		uint32_t logical_page_address,
		uint32_t physical_page_address,
		int flag){
	
	/*Write Your Own Code*/

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
	perf_inc_page_reads();
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

