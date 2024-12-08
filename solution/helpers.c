#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "wfs.h"

#define OP_SUCCESS 0
#define OP_FAIL 1
#define SYS_ERROR 2
#define INVALID -1

#define MAX_DENTRIES_IN_DBLOCK BLOCK_SIZE/sizeof(struct wfs_dentry)

#define DEBUG 1 // TODO-JP Delete. Added this just to avoid warnings from VS Code

#define BLOCK_ALIGN(x) ((((x) + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE)

#define DEBUG_PRINT(...) \
    do { if (DEBUG) {printf(__VA_ARGS__);} } while(0)
    // do { if (DEBUG) {printf("[DEBUG] ");printf(__VA_ARGS__);} } while(0)

#define ASSERT(cond, msg)                           \
    do {                                            \
        if (!(cond)) {                              \
            fprintf(stderr, "Assertion failed: %s\n", msg); \
            fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__); \
            exit(EXIT_FAILURE);                     \
        }                                           \
    } while (0)

void print_inode_deets(struct wfs_inode *inode)
{
	printf("+------------------------------------------+\n");
    printf("|              Inode details               |\n");
    printf("+------------------------------------------+\n");
    printf("num = %d\n", inode->num);
    printf("mode = %d\n", inode->mode);
    printf("size = %ld\n", inode->size);
    printf("uid = %d\n", inode->uid);
    printf("gid = %d\n", inode->gid);
    printf("nlinks = %d\n", inode->nlinks);
    printf("atim = %ld\n", inode->atim);
    printf("atim = %ld\n", inode->mtim);
    printf("atim = %ld\n", inode->ctim);
    printf("+------------------------------------------+\n");
}

void print_superblock_deets(struct wfs_sb *sb)
{
    if(sb == 0){
        printf("sb is null\n");
        return;
    }

	printf("+------------------------------------------+\n");
    printf("|              Superblock details          |\n");
    printf("+------------------------------------------+\n");
    printf("disk_id = %d\n", sb->disk_id);
    printf("num_inodes = %ld\n", sb->num_inodes);
    printf("num_data_blocks = %ld\n", sb->num_data_blocks);
    printf("i_bitmap_ptr = %ld\n", sb->i_bitmap_ptr);
    printf("d_bitmap_ptr = %ld\n", sb->d_bitmap_ptr);
    printf("i_blocks_ptr = %ld\n", sb->i_blocks_ptr);
    printf("d_blocks_ptr = %ld\n", sb->d_blocks_ptr);
    printf("disk_size = %d\n", sb->disk_size);
    printf("raid = %d\n", sb->raid);
    printf("disk_ct = %d\n", sb->disk_ct);
    printf("+------------------------------------------+\n");
}

/// @brief Fetch the root inode pointer from superblock
/// @param sb superblock pointer
/// @return 
/// @return typecasted i_blocks_ptr 
///

/// @brief Fetch the inode of the mentioned path
/// @param path 
/// @param inode 
/// @return SUCCESS if inode exists
///			FAILURE if inode doesn't exist
///
// int navigate_to_inode(const char *path, struct wfs_inode *inode)
// {

// 	// Handle 
// 	// asdasd/ 
// 	// /asdasd
// 	// ./sdasd

// 	// 

// 	return;
// }

int is_empty(const char *str) {
    return str[0] == '\0';  
}

struct wfs_sb *get_sb(char *disk)
{
    if(disk == NULL)
    {
        return NULL;
    }

    return (struct wfs_sb *)disk;
}

struct wfs_inode* get_root_inode(char *disk)
{
    struct wfs_sb *sb = get_sb(disk);

    if(sb == NULL)
    {
        return NULL;
    }
	return (struct wfs_inode *) disk + sb->i_blocks_ptr;
}

struct wfs_inode* get_inode(char *disk, int inode_num)
{
    struct wfs_sb* sb = get_sb(disk);

    if(sb == NULL)
    {
        return NULL;
    }

    if(inode_num < 0 || inode_num >= sb->num_inodes)
    {
        return NULL;
    }

    off_t base_inode_ptr = sb->i_blocks_ptr;

    off_t offset_within_disk = base_inode_ptr + BLOCK_SIZE * inode_num;

	DEBUG_PRINT("base inode: %ld\n", base_inode_ptr);
	DEBUG_PRINT("offset_within_disk: %ld\n", offset_within_disk);
	DEBUG_PRINT("disk starting address: %p\n", disk);
	DEBUG_PRINT("final pointer address: %p\n", (void *)(disk + offset_within_disk));

    return (struct wfs_inode *)(disk + offset_within_disk);
}


/// @brief Copies the input inode block to the corresponding inode block on disk. This assumes
/// @param disk 
/// @param inode 
/// @return 
///     OP_SUCCESS if copy was successful
///     OP_SUCCESS if copy failed
///
int set_inode(char* disk, struct wfs_inode* inode)
{
    // struct wfs_sb *sb = get_sb(disk);

    int inode_num = inode->num;

	struct wfs_inode *inode_ptr_on_disk = get_inode(disk, inode_num);
    
    int bytes_to_copy = sizeof(struct wfs_inode);
    memcpy(inode_ptr_on_disk, inode, bytes_to_copy);

    return OP_SUCCESS;
}

/// @brief Given a bitmap, check if a bit is set at a given byte and at a particular bit in the byte
/// @param bitmap 
/// @param byte 
/// @param bit_no 
/// @return 
int is_bit_set(char* bitmap, int bit_no)
{
    
    int byte = bit_no / 8;
    int bit_in_byte = bit_no % 8;
    // DEBUG_PRINT("Bitmap address: %p\n", (void*)bitmap);
    // DEBUG_PRINT("Byte index: %d\n", byte);
    // DEBUG_PRINT("Bit number: %d\n", bit_in_byte);

	if (bitmap == NULL) {
        fprintf(stderr, "Bitmap is NULL\n");
        return -1;
    }

	unsigned char res = (bitmap[byte] & (1 << bit_in_byte)) ? 1 : 0;
    return res;
}


int is_last_block_full(struct wfs_inode* inode)
{
    ASSERT(inode != NULL, "is_block_full: disk/inode is full");

    return inode->size%BLOCK_SIZE ? 0 : 1;
}

/// @brief Given a bitmap, set the bit at the given byte mentioned to the specified value
/// @param bitmap 
/// @param byte 
/// @param bit_no 
/// @return 
///     OP_SUCCESS if bit was set
///     OP_FAIL if bitmap is NULL or if val is not 0 or 1
int set_bit(char* bitmap, int bit_no, int val)
{

	if (bitmap == NULL) {
        fprintf(stderr, "Bitmap is NULL\n");
        return OP_FAIL;
    }

    int byte = bit_no / 8;
    int bit_in_byte = bit_no % 8;

    // DEBUG_PRINT("Bitmap address: %p\n", (void*)bitmap);
    // DEBUG_PRINT("Byte index: %d\n", byte);
    // DEBUG_PRINT("Bit number: %d\n", bit_no);

	if(val == 0)
		bitmap[byte] &= ~(1 << bit_in_byte);
	else if(val == 1)
		bitmap[byte] |= (1 <<bit_in_byte);
	else{
		fprintf(stderr, "value can be either 0 or 1\n");
		return OP_FAIL;
	}

	return OP_SUCCESS;
}

/// @brief Fetch the i_bitmap_ptr address. Fetches the address and not the
/// offset in disk
/// @param disk 
/// @return 
char *get_ibitmap(char *disk)
{
    struct wfs_sb *sb  = get_sb(disk);

    return disk + sb->i_bitmap_ptr;
}

/// @brief Fetch the d_bitmap_ptr address. Fetches the address and not the
/// offset in disk
/// @param disk 
/// @return 
char *get_dbitmap(char *disk)
{
    struct wfs_sb *sb  = get_sb(disk);

    return disk + sb->d_bitmap_ptr;
}


/// @brief Find free inode by scanning the ibitmap. Find the first bit that is
/// not set to 1
/// @param disk 
/// @return 
///     bit number between (0-num_inodes) if any of the bits are not set
///     -1 if all bits are set    
int find_free_inode(char *disk)
{
    struct wfs_sb *sb = get_sb(disk);

    char *i_bitmap_ptr = get_ibitmap(disk);

    for(int i = 0; i < sb->num_inodes; i++)
    {
        if(!is_bit_set(i_bitmap_ptr, i))
        {
            return i;
        }
    }

    return -1;
}


/// @brief Find free inode by scanning the ibitmap. Find the first bit that is
/// not set to 1
/// @param disk 
/// @return 
///     bit number between (0-num_inodes) if any of the bits are not set
///     -1 if all bits are set    
int find_free_dblock(char *disk)
{
    struct wfs_sb *sb = get_sb(disk);

    char *d_bitmap_ptr = get_dbitmap(disk);

    for(int i = 0; i < sb->num_data_blocks; i++)
    {
        if(!is_bit_set(d_bitmap_ptr, i))
        {
            return i;
        }
    }

    return -1;
}

/// @brief Fetches the number of blocks that are currently in use for an inode
/// @param inode 
/// @return 
int get_blocks_in_use_in_inode(struct wfs_inode *inode)
{
    ASSERT(inode != NULL, "get_blocks_in_use_in_inode: inode is NULL");
    
    int size = inode->size;

    return (size + BLOCK_SIZE - 1)/BLOCK_SIZE;
}


/// @brief Fetches the number of dentries are present in an inode
/// @param inode 
/// @return No. of dentries in an inode
/// 
int get_dentry_count_in_dir(struct wfs_inode *inode)
{
    ASSERT(inode != NULL, "get_dentry_count_in_dir: inode is NULL");

    return (inode->size/(sizeof(struct wfs_dentry)));
}

/// @brief 
/// @param inode_num 
/// @return 
///     OP_SUCCESS if operation was successful
///     OP_FAIL if operation was failure
int set_bit_ibitmap(char *disk, int inode_num)
{
    struct wfs_sb *sb = get_sb(disk);
    // if(sb == NULL)
    // {
    //     return OP_FAIL;
    // }

    ASSERT(sb != NULL, "set_bit_ibitmap: sb is NULL");

    if(inode_num < 0 || inode_num >= sb->num_inodes)
    {
        return OP_FAIL;
    }

    char *ibitmap = get_ibitmap(disk);

    set_bit(ibitmap, inode_num, 1);

    return OP_SUCCESS;
}

/// @brief 
/// @param inode_num 
/// @return 
///     OP_SUCCESS if operation was successful
///     OP_FAIL if operation was failure
int set_bit_ibitmap_mirrored(int inode_num)
{
    for(int i = 0; i < wfs_params.disks_ct; i++)
    {
        int res = set_bit_ibitmap(disk_mmaps[i], inode_num);

        ASSERT(res != OP_FAIL, "set_bit_ibitmap_mirrored: Setting ibitmap failed");
    }
    return OP_SUCCESS;
}

/// @brief 
/// @param inode_num 
/// @return 
///     OP_SUCCESS if operation was successful
///     OP_FAIL if operation was failure
int set_bit_dbitmap(char *disk, int block_no)
{
    // char *dbitmap = get_dbitmap(disk_mmaps[i]);

    // set_bit(dbitmap, block_no, 1);
    struct wfs_sb *sb = get_sb(disk);
    
    ASSERT(sb != NULL, "set_bit_dbitmap: sb is NULL");

    if(block_no < 0 || block_no >= sb->num_inodes)
    {
        return OP_FAIL;
    }

    // ASSERT(sb != NULL, "set_bit_dbitmap: Invalid block number");
    
    char *dbitmap = get_dbitmap(disk);

    set_bit(dbitmap, block_no, 1);
    return OP_SUCCESS;
}

/// @brief 
/// @param block_no 
/// @return 
int set_bit_dbitmap_mirrored(int block_no)
{
    for(int i = 0; i < wfs_params.disks_ct; i++)
    {
        set_bit_dbitmap(disk_mmaps[i], block_no);
    }

    return OP_SUCCESS;
}

int bytes_available_in_last_block(struct wfs_inode *inode)
{
    ASSERT(inode != NULL, "bytes_available_in_last_block: inode is NULL");
    return (BLOCK_SIZE - inode->size%BLOCK_SIZE);
}

/// @brief Identify which disk a block belongs to. Applicable only to raid0
/// @param block_no 
/// @return 
///     Disk number 0 to disks_ct - 1
int locate_disk_by_block_raid0(int block_no)
{
    ASSERT(block_no < 0, "locate_disk_by_block_raid0: Invalid block number");

    return block_no % wfs_params.disks_ct;
}

/// @brief Just does a calculation using inode size to figure which block and
/// where in block there is free space
/// @param disk 
/// @param inode 
/// @return 
///     Pointer to the free space on that particular disk
struct wfs_dentry *find_free_dentry(char* disk, struct wfs_inode* inode)
{

    // TODO-JP In mkdir, check if there is space to add a dentry. If not return ENOSPC(as per README)
    int cur_dentries = get_dentry_count_in_dir(inode);

    DEBUG_PRINT("find_free_dentry: Current no. of dentries %d\n", cur_dentries);

    int block = get_blocks_in_use_in_inode(inode);
    // int block = cur_dentries * sizeof(struct wfs_dentry) / BLOCK_SIZE;

    int offset_within_block = cur_dentries * sizeof(struct wfs_dentry) % BLOCK_SIZE;

    // TODO-JP Check for out of bounds. Not v improtant atm. There are plenty of dentries available.
    // Won't work for 17th folder/ second block dentries
    
    struct wfs_dentry *free_addr = (struct wfs_dentry *) disk + inode->blocks[block] + offset_within_block;

    return free_addr;
}


/// @brief Copy input dentry to a free slot in dblock of an inode
/// @param disk 
/// @param inode 
/// @param dentry 
/// @return 
int add_dentry_in_inode(char* disk, struct wfs_inode *inode, struct wfs_dentry *dentry)
{
    ASSERT(disk != NULL && inode != NULL, "add_dentry_in_inode: disk/inode is NULL");
    
    struct wfs_dentry *free_dentry = find_free_dentry(disk, inode);

    memcpy(free_dentry, dentry, sizeof(struct wfs_dentry));

    return OP_SUCCESS;
}

int add_dentry_in_inode_common(struct wfs_inode *inode, struct wfs_dentry *dentry)
{
    if(wfs_params.raid == 0)
    {
        int block_ct = get_blocks_in_use_in_inode(inode);
        
        if(bytes_available_in_last_block(inode) >= sizeof(struct wfs_dentry))
        {
            int disk_no = locate_disk_by_block_raid0(block_ct - 1); // Pass the last block id
            
            add_dentry_in_inode(disk_mmaps[disk_no], inode, dentry);
        }
        else
        {
            // Use next block. It also means data block needs to be allocated in
            // the next disk

            int disk_no = locate_disk_by_block_raid0(block_ct); // Pass the last block id
            
            int new_block_no = find_free_dblock(disk_mmaps[block_ct]);

            set_bit_dbitmap(disk_mmaps[disk_no], new_block_no);

            add_dentry_in_inode(disk_mmaps[disk_no], inode, dentry);
        }

        return OP_SUCCESS;
    }

    // Raid 1
    // int block_ct = get_blocks_in_use_in_inode(inode);
    
    if(bytes_available_in_last_block(inode) >= sizeof(struct wfs_dentry))
    {
        for(int i = 0; i < wfs_params.disks_ct; i++)
            add_dentry_in_inode(disk_mmaps[i], inode, dentry);
    }
    else
    {
        ASSERT(0, "add_dentry_in_inode: TODO - Unhandled >16 folders case");
        // TODO-JP Complete this later

        // Use next block. It also means data block needs to be allocated in
        // the next disk

        // int disk_no = locate_disk_by_block_raid0(block_ct); // Pass the last block id
        
        // int new_block_no = find_free_dblock(disk_mmaps[block_ct]);

        // set_bit_dbitmap(disk_mmaps[disk_no], new_block_no);

        // add_dentry_in_inode(disk_mmaps[disk_no], inode, dentry);
    }

    return OP_SUCCESS;    
}

/// @brief Fetch the block at the mentioned offset. If the offset is not block
/// aligned, it returns the pointer to the next block aligned address
/// @param disk 
/// @param block_offset 
/// @return 
char *get_disk_block(char *disk, off_t block_offset)
{
    struct wfs_sb *sb = get_sb(disk);

    // Check for out of bound access
    //
    if(block_offset >= sb->disk_size)
        return NULL;

    return disk + BLOCK_ALIGN(block_offset);
}

/// @brief Fetch the virtual address of a block
/// @param disk 
/// @param block_offset 
/// @return 
char *get_disk_block_v2(char *disk, int block)
{
    struct wfs_sb *sb = get_sb(disk);

    // Check for out of bound access
    //
    if(block >= sb->disk_size/BLOCK_SIZE)
        return NULL;
    
    return disk + BLOCK_SIZE*block;
}



/// @brief Adds the new datablock details to the inode blocks array
/// @param inode 
/// @param inode_block_index 
/// @param free_block_id 
/// @return 
int set_inode_block(struct wfs_inode *inode, int inode_block_index, int free_block_id)
{

    ASSERT(inode != NULL, "set_inode_block: inode is NULL");

    struct wfs_sb *sb = get_sb(disk_mmaps[0]); // TODO-JP Not ideal to hardcode. Need to find a better approach

    inode->blocks[inode_block_index] = sb->d_blocks_ptr + free_block_id*BLOCK_SIZE;

    for(int i = 0; i < wfs_params.disks_ct; i++)
    {
        set_inode(disk_mmaps[i], inode);
    }

    return OP_SUCCESS;
}


/// @brief Tokenizes/splits the path and returns an array of the path parts
/// eg. /a/b/c = [a,b,c].
/// @param path Path to be tokenized/split
/// @return 
///     Pointer to the array if path was successfully tokenized
///     OP_FAIL if tokenizing failed
///
/// NOTE: Responsibility of the caller to free the array returned
///
char** tokenize_path(char* path)
{
    DEBUG_PRINT("Input path to tokenize: %s\n", path);

    // Array to store path components/segements
    //
    char **path_arr = malloc(MAX_PATH_DEPTH*sizeof(char*));

    // Initialize all entries to NULL
    //
    for(int i = 0; i < MAX_PATH_DEPTH; i++)
    {
        path_arr[i] = NULL;
    }

    int path_parts_ct = 0;
    
    DEBUG_PRINT("Input path to tokenize: %s\n", path);
    char *path_dup = strdup(path);

    if(path_dup == NULL)
    {
        fprintf(stderr, "strdup path failed\n");
        return NULL;
    }
    
    char *path_part;

    path_part = strtok(path_dup, "/");
    
    while (path_part != NULL && path_parts_ct < MAX_PATH_DEPTH) {
        path_arr[path_parts_ct] = path_part;
        path_parts_ct++;
        
        path_part = strtok(NULL, "/");
    }    

    DEBUG_PRINT("Path: ");
    for (int i = 0; i < path_parts_ct; i++) {
        DEBUG_PRINT("%s ", path_arr[i]);
    }
    DEBUG_PRINT("\n");
    
    return path_arr;
}

// get_inode_block_offsets(struct wfs_inode *inode)
// {
//     off_t blocks[N_BLOCKS];

//     return 
// }

/// @brief Fetches the inode num if the target exists in an inode
/// @param disk 
/// @param inode 
/// @param target 
/// @return 
int is_exists_in_dir(char *disk, struct wfs_inode* inode, char *target)
{

    ASSERT(inode != NULL, "is_exists_in_dir: inode is NULL");

    int blocks_ct = get_blocks_in_use_in_inode(inode);

    DEBUG_PRINT("Blocks in use for inode %d - %d\n", inode->num, blocks_ct);

    int dentries_ct = get_dentry_count_in_dir(inode);
    
    DEBUG_PRINT("Dentries in inode %d - %d\n", inode->num, dentries_ct);
    
    for(int i = 0; i < blocks_ct; i++)
    {
        DEBUG_PRINT("Inode block entry: %d - %ld\n", i, inode->blocks[i]);

        ASSERT(inode->blocks[i] != 0, "is_exists_in_dir: Block shouldn't be 0");
        
        struct wfs_dentry *dentry = (struct wfs_dentry *)disk + inode->blocks[i];
        
        for(int i = 0; i < MAX_DENTRIES_IN_DBLOCK; i++) 
        {
            if(dentries_ct == 0)
            {
                // Didn't find any entry
                //
                return INVALID;
            }

            // Point to the correct dentry in block
            //
            dentry += i; 

            DEBUG_PRINT("Dentry pointer: %p\n", (void *)dentry);
            DEBUG_PRINT("Dentry content: %s", (char *)dentry);
            DEBUG_PRINT("Block offset: %ld\n", inode->blocks[i]);

            if(strncpy(target, dentry->name, MAX_NAME) == 0)
            {
                // Found match
                return dentry->num;
            }
            dentries_ct--;
        }
    }

    return INVALID;
}

int mkdir_wfs(char *disk, char* path) //, mode_t mode)
{
    struct wfs_inode *root_inode = get_root_inode(disk); 

    if(root_inode == NULL)
    {
        return OP_FAIL;
    }
    
    char **path_arr = tokenize_path(path);

    int inode_num = is_exists_in_dir(disk, root_inode, path_arr[0]);
    if(inode_num != -1)
    {
        DEBUG_PRINT("Path %s exists\n", path_arr[0]);
    }
    else
    {
        DEBUG_PRINT("Path %s does not exist", path_arr[0]);
        
        int free_inode_num = find_free_inode(disk);
        DEBUG_PRINT("Free inode slot at %d\n", free_inode_num);

        // ibitmap data should be same across disks irrespective of raid mode
        //
        set_bit_ibitmap_mirrored(free_inode_num);
    }

    // for(int i = 0; i < MAX_PATH_DEPTH; i++)
    // {
    //     // get_disk_block(disk, inode)

    //     if(is_exists_in_dir(disk, root_inode, path_arr[0]))
    //     {
    //         DEBUG_PRINT("Path %s exists\n", path_arr[i]);
    //     }
    //     else
    //     {
    //         DEBUG_PRINT("Path exists");
    //     }

    // }
    return 1;
}

// navigate_to_path_inode()