#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "wfs.h"

#define OP_SUCCESS 0
#define OP_FAIL 1
#define SYS_ERROR 2

#define DEBUG 1 // TODO-JP Delete. Added this just to avoid warnings from VS Code

#define BLOCK_ALIGN(x) ((((x) + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE)

#define DEBUG_PRINT(...) \
    do { if (DEBUG) printf(__VA_ARGS__); } while(0)


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
struct wfs_inode* get_root_inode(struct wfs_sb* sb)
{
	return (struct wfs_inode *) sb->i_blocks_ptr;
}

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

struct wfs_sb *get_sb(char *disk)
{
    if(disk == NULL)
    {
        return NULL;
    }

    return (struct wfs_sb *)disk;
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


    // TODO-JP check for lower and upper bound range of byte

	unsigned char res = (bitmap[byte] & (1 << bit_in_byte)) ? 1 : 0;
    return res;
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