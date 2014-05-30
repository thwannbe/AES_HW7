#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "blueftl_ftl_base.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_mapping_page.h"
#include "blueftl_gc_page.h"
#include "blueftl_util.h"

#include<math.h>
#include "blueftl_mapping_dftl_page.h"

struct ftl_base_t ftl_base_dftl_mapping = {
	.ftl_create_ftl_context = dftl_mapping_create_ftl_context,
	.ftl_destroy_ftl_context = dftl_mapping_destroy_ftl_context,
	.ftl_get_mapped_physical_page_address = dftl_mapping_get_mapped_physical_page_address,
	.ftl_map_logical_to_physical = dftl_mapping_map_logical_to_physical,
	.ftl_get_free_physical_page_address = dftl_mapping_get_free_physical_page_address,
	.ftl_trigger_gc = gc_dftl_trigger_gc,
	.ftl_trigger_merge = NULL,
	.ftl_trigger_wear_leveler = NULL,
};

//initialize the DFTL
uint32_t init_dftl(struct ftl_context_t* ptr_ftl_context){

	/* Write Your Own Code */
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = 
		(struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping;
	if((ptr_pg_mapping->ptr_dftl_table->ptr_cached_mapping_table = 
				(struct dftl_cached_mapping_entry_t**)malloc(sizeof(struct dftl_cached_mapping_entry_t*) * CMT_MAX)) == NULL) {
		printf ("blueftl_mapping_page: there is no enough memory that will be used for an dftl CMT\n");
	}
	if((ptr_pg_mapping->ptr_dftl_table->ptr_global_translation_directory = (uint32_t*)malloc(sizeof(uint32_t) * 128)) == NULL) {
		printf ("blueftl_mapping_page: there is no enough memory that will be used for an dftl GTD\n");
	}
	ptr_pg_mapping->ptr_dftl_table->nr_cached_mapping_table_entries = 0;

	return 0;
}

uint32_t destroy_dftl(struct dftl_context_t* ptr_dftl_context){

	/* Write Your Own Code */
	uint32_t i;

	for(i = 0; i < CMT_MAX; i++) {
		if(ptr_dftl_context->ptr_cached_mapping_table[i] != NULL) {
			free(ptr_dftl_context->ptr_cached_mapping_table[i]);
		}
	}
	if(ptr_dftl_context->ptr_cached_mapping_table != NULL)
		free(ptr_dftl_context->ptr_cached_mapping_table);

	if(ptr_dftl_context->ptr_global_translation_directory != NULL)
		free(ptr_dftl_context->ptr_global_translation_directory);

	return 0;
}

uint32_t init_translation_blocks(struct ftl_context_t* ptr_ftl_context)
{
	uint32_t loop_bus, loop_chip;
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = 
		(struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping;
	struct flash_block_t* ptr_erase_block;

	for (loop_bus = 0; loop_bus < ptr_ssd->nr_buses; loop_bus++) 
	{
		for (loop_chip = 0; loop_chip < ptr_ssd->nr_chips_per_bus; loop_chip++) 
		{
			if ((ptr_erase_block = ssdmgmt_get_free_block (ptr_ssd, loop_bus, loop_chip))) 
			{
				*(ptr_pg_mapping->ptr_translation_blocks + (loop_bus * ptr_ssd->nr_chips_per_bus + loop_chip)) = ptr_erase_block;
			} 
			else 
			{
				// OOPS!!! ftl needs a garbage collection.
				printf ("blueftl_mapping_page: there is no free block that will be used for an translation block\n");
			}
		}
	}

	return 0;
}

uint32_t init_gc_tblocks(struct ftl_context_t* ptr_ftl_context)
{
	uint32_t loop_chip, loop_bus;

	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = 
		(struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping;
	struct flash_block_t* ptr_erase_block;

	for (loop_bus = 0; loop_bus < ptr_ssd->nr_buses; loop_bus++) 
	{
		for (loop_chip = 0; loop_chip < ptr_ssd->nr_chips_per_bus; loop_chip++) 
		{
			if ((ptr_erase_block = ssdmgmt_get_free_block(ptr_ssd, loop_bus, loop_chip))!= NULL) 
			{
				*(ptr_pg_mapping->ptr_gc_tblocks + (loop_bus * ptr_ssd->nr_chips_per_bus + loop_chip)) = ptr_erase_block;
				ptr_erase_block->is_reserved_block = 1;
			} 
			else 
			{
				// OOPS!!! ftl needs a garbage collection.
				printf ("blueftl_mapping_page: there is no free block that will be used for a gc tblock\n");
			}
		}
	}

	return 0;
}

uint32_t init_gc_blocks(struct ftl_context_t* ptr_ftl_context)
{
	uint32_t loop_chip, loop_bus;

	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = 
		(struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping;
	struct flash_block_t* ptr_erase_block;

	for (loop_bus = 0; loop_bus < ptr_ssd->nr_buses; loop_bus++) 
	{
		for (loop_chip = 0; loop_chip < ptr_ssd->nr_chips_per_bus; loop_chip++) 
		{
			if ((ptr_erase_block = ssdmgmt_get_free_block(ptr_ssd, loop_bus, loop_chip))!= NULL) 
			{
				*(ptr_pg_mapping->ptr_gc_blocks + (loop_bus * ptr_ssd->nr_chips_per_bus + loop_chip)) = ptr_erase_block;
				ptr_erase_block->is_reserved_block = 1;
			} 
			else 
			{
				// OOPS!!! ftl needs a garbage collection.
				printf ("blueftl_mapping_page: there is no free block that will be used for a gc block\n");
			}
		}
	}

	return 0;
}

uint32_t init_active_blocks(struct ftl_context_t* ptr_ftl_context)
{
	uint32_t loop_bus, loop_chip;
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = 
		(struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping;
	struct flash_block_t* ptr_erase_block;

	for (loop_bus = 0; loop_bus < ptr_ssd->nr_buses; loop_bus++) 
	{
		for (loop_chip = 0; loop_chip < ptr_ssd->nr_chips_per_bus; loop_chip++) 
		{
			if ((ptr_erase_block = ssdmgmt_get_free_block (ptr_ssd, loop_bus, loop_chip))) 
			{
				*(ptr_pg_mapping->ptr_active_blocks + (loop_bus * ptr_ssd->nr_chips_per_bus + loop_chip)) = ptr_erase_block;
			} 
			else 
			{
				// OOPS!!! ftl needs a garbage collection.
				printf ("blueftl_mapping_page: there is no free block that will be used for an active block\n");
			}
		}
	}

	return 0;
}

uint32_t init_ru_blocks (struct ftl_context_t* ptr_ftl_context)
{
	uint32_t loop_bus;
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = 
		(struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping;

	for (loop_bus = 0; loop_bus < ptr_ssd->nr_buses; loop_bus++) 
	{
		ptr_pg_mapping->ptr_ru_chips[loop_bus] = ptr_ssd->nr_chips_per_bus - 1;
	}
	ptr_pg_mapping->ru_bus = ptr_ssd->nr_buses - 1;

	return 0;
}

struct ftl_context_t* dftl_mapping_create_ftl_context (struct virtual_device_t* ptr_vdevice)
{
	struct ftl_context_t* ptr_ftl_context = NULL;
	struct flash_ssd_t* ptr_ssd = NULL;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = NULL;

	printf ("blueftl_mapping_page: Initialized Start!\n");
	/* create the ftl context */
	if ((ptr_ftl_context 
				= (struct ftl_context_t*) malloc (sizeof (struct ftl_context_t))) == NULL) 
	{
		printf ("blueftl_mapping_page: the creation of the ftl context failed\n");
		goto error_alloc_ftl_context;
	}

	/* create the ssd context */
	if ((ptr_ftl_context->ptr_ssd = ssdmgmt_create_ssd (ptr_vdevice)) == NULL) 
	{
		printf ("blueftl_mapping_page: the creation of the ftl context failed\n");
		goto error_create_ssd_context;
	}
	
	/* create the page mapping context */
	if ((ptr_ftl_context->ptr_mapping 
				= (struct ftl_page_mapping_context_t *) malloc (sizeof (struct ftl_page_mapping_context_t))) == NULL) 
	{
		printf ("blueftl_mapping_page: the creation of the ftl context failed\n");
		goto error_alloc_ftl_page_mapping_context;
	}
	ptr_ssd = (struct flash_ssd_t*)ptr_ftl_context->ptr_ssd;
	ptr_pg_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;

	/* allocate the memory for the dftl table */
	if ((ptr_pg_mapping->ptr_dftl_table = 
			(struct dftl_context_t*) malloc (sizeof (struct dftl_context_t))) == NULL) 
	{
		printf ("blueftl_mapping_page: error occurs when allocating dftl table\n");
		goto error_alloc_dftl_table;
	}
	
	init_dftl (ptr_ftl_context);
	
	/* allocate the memory for the translation blocks */
	if ((ptr_pg_mapping->ptr_translation_blocks = 
			(struct flash_block_t**) malloc (sizeof (struct flash_block_t*) * ptr_ssd->nr_buses * ptr_ssd->nr_chips_per_bus)) == NULL) 
	{
		printf ("blueftl_mapping_page: error occurs when allocating translation blocks\n");
		goto error_alloc_translation_block;
	}
	
	init_translation_blocks (ptr_ftl_context);

	/* allocate the memory for the gc tblocks */
	if ((ptr_pg_mapping->ptr_gc_tblocks = 
			(struct flash_block_t**) malloc (sizeof (struct flash_block_t*) * ptr_ssd->nr_buses * ptr_ssd->nr_chips_per_bus)) == NULL) 
	{
		printf ("blueftl_mapping_page: error occurs when allocating gc tblocks\n");
		goto error_alloc_gc_tblock;
	}
	
	init_gc_tblocks (ptr_ftl_context);

	/* allocate the memory for the gc blocks */
	if ((ptr_pg_mapping->ptr_gc_blocks = 
			(struct flash_block_t**) malloc (sizeof (struct flash_block_t*) * ptr_ssd->nr_buses * ptr_ssd->nr_chips_per_bus)) == NULL) 
	{
		printf ("blueftl_mapping_page: error occurs when allocating gc blocks\n");
		goto error_alloc_gc_block;
	}
	
	init_gc_blocks (ptr_ftl_context);

	/* allocate the memory for the active blocks */
	if ((ptr_pg_mapping->ptr_active_blocks = 
			(struct flash_block_t**) malloc (sizeof (struct flash_block_t*) * ptr_ssd->nr_buses * ptr_ssd->nr_chips_per_bus)) == NULL) 
	{
		printf ("blueftl_mapping_page: error occurs when allocating active blocks\n");
		goto error_alloc_active_block;
	}
	init_active_blocks (ptr_ftl_context);

	/* allocate the memory for the ru blocks */
	if ((ptr_pg_mapping->ptr_ru_chips =
			(uint32_t*) malloc (sizeof (uint32_t) * ptr_ssd->nr_buses)) == NULL) 
	{
		printf ("blueftl_mapping_page: the memory allocation of the ru blocks failed\n");
		goto error_alloc_ru_blocks;
	}
	init_ru_blocks (ptr_ftl_context);

	printf ("blueftl_mapping_page: Initialized Success!\n");
	/* set virtual device */
	ptr_ftl_context->ptr_vdevice = ptr_vdevice;


	return ptr_ftl_context;

error_alloc_ru_blocks:
	free (ptr_pg_mapping->ptr_active_blocks);

error_alloc_active_block:
	free (ptr_pg_mapping->ptr_gc_blocks);

error_alloc_gc_block:
	free (ptr_pg_mapping->ptr_gc_tblocks);

error_alloc_gc_tblock:
	free (ptr_pg_mapping->ptr_translation_blocks);

error_alloc_translation_block:
	free (ptr_pg_mapping->ptr_dftl_table);

error_alloc_dftl_table:

error_alloc_mapping_table_info:
	free (ptr_pg_mapping);

error_alloc_ftl_page_mapping_context:
	ssdmgmt_destroy_ssd (ptr_ssd);

error_create_ssd_context:
	free (ptr_ftl_context);

error_alloc_ftl_context:
	return NULL;
}

void dftl_mapping_destroy_ftl_context(struct ftl_context_t* ptr_ftl_context)
{
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = 
		(struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;
	int loop;

	/* destroy ru blocks */
	if (ptr_pg_mapping->ptr_ru_chips != NULL) 
	{
		free (ptr_pg_mapping->ptr_ru_chips);
	}

	/* destroy active blocks */
	if (ptr_pg_mapping->ptr_active_blocks != NULL) 
	{
		free (ptr_pg_mapping->ptr_active_blocks);
	}

	/* destroy gc blocks */
	if (ptr_pg_mapping->ptr_gc_blocks != NULL) 
	{
		free (ptr_pg_mapping->ptr_gc_blocks);
	}

	if (ptr_pg_mapping->ptr_dftl_table != NULL)
	{
		destroy_dftl(ptr_pg_mapping->ptr_dftl_table);
		free (ptr_pg_mapping->ptr_dftl_table);
	}

	if (ptr_pg_mapping->ptr_translation_blocks != NULL)
	{	
		for(loop = 0; loop < NR_TRANS; loop++){
			if(ptr_pg_mapping->ptr_translation_blocks[loop] != NULL) {
				free(ptr_pg_mapping->ptr_translation_blocks + loop);
			}
		}
	}

	/* destroy the ftl context */
	ssdmgmt_destroy_ssd (ptr_ssd);

	if (ptr_pg_mapping != NULL) 
	{
		free (ptr_pg_mapping);
	}

	if (ptr_ftl_context != NULL) 
	{
		free (ptr_ftl_context);
	}
}

// functions for managing mapping table
uint32_t get_new_free_page_addr(struct ftl_context_t* ptr_ftl_context)
{
	uint32_t curr_bus, curr_chip;
	uint32_t curr_block, curr_page;
	uint32_t curr_physical_page_addr;

	struct flash_block_t* ptr_active_block = NULL;
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = 
		(struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping;

	// (1) choose a bus to be written
	if (ptr_pg_mapping->ru_bus == (ptr_ssd->nr_buses - 1))
	{
		ptr_pg_mapping->ru_bus = 0;
		
		curr_bus = ptr_pg_mapping->ru_bus;
		// (2) choose a chip to be written
		if (ptr_pg_mapping->ptr_ru_chips[curr_bus] == (ptr_ssd->nr_chips_per_bus - 1))
		{
			ptr_pg_mapping->ptr_ru_chips[curr_bus] = 0;
		}
		else
		{
			ptr_pg_mapping->ptr_ru_chips[curr_bus]++;
		}
		
	}
	else
	{
		ptr_pg_mapping->ru_bus++;
		curr_bus = ptr_pg_mapping->ru_bus;
	}
	
	curr_chip = ptr_pg_mapping->ptr_ru_chips[curr_bus];

	// (3) see if the bus number calculated is different from the real bus number
	ptr_active_block = *(ptr_pg_mapping->ptr_active_blocks + (curr_bus * ptr_ssd->nr_chips_per_bus + curr_chip));

	if (ptr_active_block->no_bus != curr_bus || ptr_active_block->no_chip != curr_chip)
	{
		printf ("blueftl_mapping_page: the bus number for the active block is not same to the current bus number\n");
		return -1;
	}

	// (4) see if there are free pages to be used in the block
	if (ptr_active_block->is_reserved_block != 0||ptr_active_block->nr_free_pages == 0) 
	{
		// there is no free page in the block, and therefore it it necessary to allocate a new log block for this bus
		ptr_active_block 
			= *(ptr_pg_mapping->ptr_active_blocks + (curr_bus * ptr_ssd->nr_chips_per_bus + curr_chip)) 
			= ssdmgmt_get_free_block(ptr_ssd, curr_bus, curr_chip);


		if (ptr_active_block != NULL) 
		{
			if (ptr_active_block->no_bus != curr_bus) 
			{
				printf ("blueftl_mapping_page: bus number or free block number is incorrect (%d %d)\n", ptr_active_block->no_bus, ptr_active_block->nr_free_pages);
				return -1;
			}
		} 
		else 
		{
			// oops! there is no free block in the bus. Hence we need a garbage collection.
			return -1;
		}
	}

	// (5) now we can know that target block and page
	curr_block = ptr_active_block->no_block;
	curr_page = ptr_ssd->nr_pages_per_block - ptr_active_block->nr_free_pages;
	
	// (3) get the physical page address from the layout information
	curr_physical_page_addr = ftl_convert_to_physical_page_address (curr_bus, curr_chip, curr_block, curr_page);

	return curr_physical_page_addr;
}


int8_t is_valid_address_range (struct ftl_context_t* ptr_ftl_context, uint32_t logical_page_address)
{
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;

	if (logical_page_address >=
			ptr_ssd->nr_buses * ptr_ssd->nr_chips_per_bus * ptr_ssd->nr_blocks_per_chip * ptr_ssd->nr_pages_per_block)
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

int32_t map_logical_to_physical(struct ftl_context_t* ptr_ftl_context, uint32_t logical_page_address, uint32_t physical_page_address, uint32_t mode)
{

	/* Write Your Own Code */

	return 0;
}

int32_t dftl_mapping_get_mapped_physical_page_address (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint32_t *ptr_bus,
	uint32_t *ptr_chip,
	uint32_t *ptr_block,
	uint32_t *ptr_page)
{

	/* Write Your Own Code */
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_page_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;

	uint32_t physical_page_address;
	int32_t ret = -1;
	
	/* obtain the physical page address using the page mapping table */
	if((physical_page_address = dftl_get_physical_address(ptr_ftl_context, logical_page_address)) == -1) {
		/* the requested logical page is not mapped to any physical page */
		*ptr_bus = *ptr_chip = *ptr_block = *ptr_page = -1;
		ret = -1;
	} else { /* the requested logical page is matched with physical page */
		struct flash_page_t* ptr_erase_page = NULL;
		
		/* decoding the physical page address */
		ftl_convert_to_ssd_layout (physical_page_address, ptr_bus, ptr_chip, ptr_block, ptr_page);
		
		ptr_erase_page = &ptr_ssd->list_buses[*ptr_bus].list_chips[*ptr_chip].list_blocks[*ptr_block].list_pages[*ptr_page];
		if (ptr_erase_page->page_status == PAGE_STATUS_FREE) {
			/* the logical page must be mapped to the corresponding physical page */
			*ptr_bus = *ptr_chip = *ptr_block = *ptr_page = -1;
			ret = -1;
		} else {
			ret = 0;
		}
	}
	return ret;
}

int32_t dftl_mapping_get_free_physical_page_address (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address,
	uint32_t *ptr_bus,
	uint32_t *ptr_chip,
	uint32_t *ptr_block,
	uint32_t *ptr_page,
	uint32_t mode) /* mode is always 0 */
{

	/* Write Your Own Code */
	int32_t ret = -1;
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;
	uint32_t physical_page_address;

	if ((physical_page_address = get_new_free_page_addr (ptr_ftl_context)) == -1) 
	{
		/* get the target bus and chip for garbage collection */
		*ptr_bus = ptr_pg_mapping->ru_bus;
		*ptr_chip = ptr_pg_mapping->ptr_ru_chips[ptr_pg_mapping->ru_bus];
		*ptr_block = -1;
		*ptr_page = -1;

		/* getting a new free page faild, so we move to the previous bus */
		if (ptr_pg_mapping->ptr_ru_chips[ptr_pg_mapping->ru_bus] == 0)
		{
			ptr_pg_mapping->ptr_ru_chips[ptr_pg_mapping->ru_bus] = ptr_ssd->nr_chips_per_bus - 1;
		}
		else
		{
			ptr_pg_mapping->ptr_ru_chips[ptr_pg_mapping->ru_bus]--;
		}

		if (ptr_pg_mapping->ru_bus == 0)
		{
			ptr_pg_mapping->ru_bus = ptr_ssd->nr_buses - 1;
		}
		else
		{
			ptr_pg_mapping->ru_bus--;
		}

		/* now, it is time to reclaim garbage */
		ret = -1;
	}
	else 
	{
		/* decoding the physical page address */
		ftl_convert_to_ssd_layout (physical_page_address, ptr_bus, ptr_chip, ptr_block, ptr_page);

		ret = 0;
	}

	return ret;
}

int32_t dftl_mapping_map_logical_to_physical (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint32_t bus,
	uint32_t chip,
	uint32_t block,
	uint32_t page,
	uint32_t mode)
{
	uint32_t physical_page_address = ftl_convert_to_physical_page_address (bus, chip, block, page);
	if(mode == 0) /* mode is always 0 */
	return map_logical_to_physical (ptr_ftl_context, logical_page_address, physical_page_address, 0);
	else
	return map_logical_to_physical (ptr_ftl_context, logical_page_address, physical_page_address, 1);
}

int32_t invalid_previous (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address)
{

	/* Write Your Own Code */

	return 0;
}

