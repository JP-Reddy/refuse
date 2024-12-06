#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "wfs.h"

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
    printf("raid = %d\n", sb->raid);
    printf("disk_ct = %d\n", sb->disk_ct);
    printf("+------------------------------------------+\n");
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
