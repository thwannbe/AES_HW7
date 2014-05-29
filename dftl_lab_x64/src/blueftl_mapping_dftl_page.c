#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "blueftl_util.h"
#include "blueftl_ftl_base.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_mapping_page.h"

#include "blueftl_dftl_list.h" //for doubly linked list
#include "blueftl_dftl_hash.h" //for linear hashing
#include "blueftl_mapping_dftl_page.h"
#include "blueftl_gc_page.h"

//insert the mapping into the CMT
static int insert_mapping(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context, uint32_t logical_page_address, uint32_t physical_page_address, int flag);

//create the new cache entry 
static struct dftl_cached_mapping_entry_t* create_entry(uint32_t dirty, uint32_t logical_page_address, uint32_t physical_page_address);

//check the CMT whether it is full
static uint32_t isFull(struct dftl_context_t* ptr_dftl_context, uint32_t logical_page_address);

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

}

//insert a new translation entry into the CMT 
uint32_t dftl_map_logical_to_physical(
		struct ftl_context_t* ptr_ftl_context,
		uint32_t logical_page_address,
		uint32_t physical_page_address){

	/*Write Your Own Code*/

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
		struct dftl_context_t* ptr_dftl_context, 
		uint32_t logical_page_address){

	/*Write Your Own Code*/

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
	uint32_t physical_page_address, curr_bus, curr_chip, curr_block, curr_page;
	uint32_t physical_translation_page_address;
	uint8_t* ptr_buff = (uint8_t*)malloc(sizeof(uint8_t) * FLASH_PAGE_SIZE); 
	
	/*Write Your Own Code*/
	
}

/* Write Back Dirty Translation Entry into Translation Page */
static uint32_t write_back_tpage(
		struct ftl_context_t* ptr_ftl_context,
		struct dftl_context_t* ptr_dftl_context,
		struct dftl_cached_mapping_entry_t* ptr_evict){

	/*Write Your Own Code*/

}

