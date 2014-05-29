#ifndef DFTL_MAPPING
#define DFTL_MAPPING

#define CMT_MAX 1024 

uint32_t GTD_MAX;

struct cache_mapping_table {
	uint32_t logical_page_address;
	uint32_t physical_page_address;
};

struct dftl_context_t {
	uint32_t nr_cached_mapping_table_entries; 	/* the number of CMT */
	struct cache_mapping_table* ptr_cached_mapping_table;			/* for the dftl cached mapping */
	uint32_t* ptr_global_translation_directory;	/* for the storing entire address informations */
};

//translate the logical address to the physical address (for the read request case)
uint32_t dftl_get_physical_address(struct ftl_context_t* ptr_ftl_context, uint32_t logical_page_address);

//insert a new translation entry into the CMT (for the write request case with a new physical page address) 
uint32_t dftl_map_logical_to_physical(struct ftl_context_t* ptr_ftl_context, uint32_t logical_page_address, uint32_t physical_page_address);

//modfiy the GTD
uint32_t dftl_modify_gtd(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context, uint32_t index, uint32_t physical_page_address); 

//update CMT
uint32_t update_cmt(struct ftl_context_t* ptr_ftl_context, struct dftl_context_t* ptr_dftl_context, uint32_t logical_page_address, uint32_t physical_page_address,
		int flag);
#endif
