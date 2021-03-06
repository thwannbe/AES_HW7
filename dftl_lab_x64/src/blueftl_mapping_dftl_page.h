#ifndef DFTL_MAPPING
#define DFTL_MAPPING

#define CMT_MAX 2048
#define GTD_FREE -1

uint32_t GTD_MAX;

struct dftl_cached_mapping_entry_t {
	struct dftl_cached_mapping_entry_t *next; /* linked-list */
	uint32_t logical_page_address;
	uint32_t physical_page_address;
	uint32_t dirty;
};

struct dftl_context_t {
	uint32_t nr_cached_mapping_table_entries; 	/* the number of CMT */
	struct dftl_cached_mapping_entry_t* ptr_cached_mapping_table_head;	/* for the dftl cached mapping */
	uint32_t* ptr_global_translation_directory;	/* for the storing entire address informations */
};

//translate the logical address to the physical address (for the read request case)
uint32_t dftl_get_physical_address(struct ftl_context_t* ptr_ftl_context, uint32_t logical_page_address, uint32_t read_write);

//insert a new translation entry into the CMT (for the write request case with a new physical page address) 
uint32_t dftl_map_logical_to_physical(struct ftl_context_t* ptr_ftl_context, uint32_t logical_page_address, uint32_t physical_page_address);

//modfiy the GTD
uint32_t dftl_modify_gtd(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context, uint32_t index, uint32_t physical_page_address); 

//update CMT
uint32_t update_cmt(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context, uint32_t logical_page_address, uint32_t physical_page_address,
		int flag);

//insert the mapping into the CMT
int insert_mapping(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context, struct dftl_cached_mapping_entry_t* entry, int new);


//for debugging
void print_curr_dftl_mapping_table(struct dftl_context_t* ptr_dftl_context);
void print_curr_dftl_gtd(struct dftl_context_t* ptr_dftl_context);
void print_page_buffer_status(uint8_t* ptr_buff);
void print_block_info(struct flash_block_t* target_block);
void block_status_checker(struct flash_ssd_t* ptr_ssd);

#endif
