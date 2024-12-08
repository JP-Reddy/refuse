#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "wfs.h"

struct fs_info {
    char disks[MAX_DISKS][MAX_NAME];
    int disks_ct;
    int inodes_ct;
    int dblocks_ct;
    int raid;
};

enum arg_param {
    DISK,
    INODE,
    DBLOCK,
    RAID
};

struct disk_info{
    struct wfs_sb sb;
    char disk_name[MAX_NAME];
};

struct fs_info my_fs;

int round_up_to_nearest_thirtytwo(int num)
{
    return num%32 == 0 ? num : (num/32 + 1)*32;
}

int verify_raid_mode(int raid)
{
    if(raid == 0 || raid == 1){
        return 1;
    }
    return 0;
}

int validate_arguments(int argc, char* argv[])
{
    // Even number of args would mean some parameter is missing
    // If there are less than seven args, some parameter among disks, inode,
    // datablocks is missing
    //
    if(argc%2 == 0 || argc < 7)
    {
        printf("Invalid number of arguments.");
        return 0;
    }

    // Long name > MAX_NAME - fail

    // Parameters to inode and dblock should be integers
    return 1;
}

int parse_arguments(int argc, char* argv[])
{
    enum arg_param arg_type;
    for(int i = 1; i < argc; i += 2)
    {
        // Identify the arg type
        //
        if(i%2 == 0)
        {
            continue;
        }
            if(strcmp(argv[i], "-d") == 0) {
                arg_type = DISK;
                printf("jpd Arg type = %d\n", arg_type);
            }
            else if(strcmp(argv[i], "-i") == 0) {
                arg_type = INODE;
                printf("jpd Arg type = %d\n", arg_type);
            }
            else if(strcmp(argv[i], "-b") == 0) {
                arg_type = DBLOCK;
                printf("jpd Arg type = %d\n", arg_type);
            }
            else if(strcmp(argv[i], "-r") == 0) {
                arg_type = RAID;
                printf("jpd Arg type = %d\n", arg_type);
            }
            else {
                return 0;
            }
        // }
        
        // {
            // Parse the arg values
            //
            if(arg_type == DISK){
                strncpy(my_fs.disks[my_fs.disks_ct], argv[i+1], MAX_NAME);
                my_fs.disks[my_fs.disks_ct][MAX_NAME - 1] = '\0';
                printf("jpd name = %s\n", my_fs.disks[my_fs.disks_ct]);
                my_fs.disks_ct++;
            }
            else if(arg_type == RAID){
                my_fs.raid = atoi(argv[i+1]);
                printf("jpd raid = %d\n", my_fs.raid);
            }
            else if(arg_type == INODE) {
                my_fs.inodes_ct = round_up_to_nearest_thirtytwo(atoi(argv[i+1]));
                printf("jpd inodes_ct = %d\n", my_fs.inodes_ct);
            }
            else if(arg_type == DBLOCK) {
                my_fs.dblocks_ct = round_up_to_nearest_thirtytwo(atoi(argv[i+1]));
                printf("jpd dblocks_ct = %d\n", my_fs.dblocks_ct);
            }
            else {
                printf("jpd hmm");
                return 0;
            }
        // }
    }

    if(!verify_raid_mode(my_fs.raid))
        return 0;

    return 1;
}

void fs_init_params(struct fs_info *my_fs)
{
    my_fs->disks_ct = 0;
    my_fs->inodes_ct = 0;
    my_fs->dblocks_ct = 0;
    my_fs->raid = -1;
}

// // Superblock
// struct wfs_sb {
//     size_t num_inodes;
//     size_t num_data_blocks;
//     off_t i_bitmap_ptr;
//     off_t d_bitmap_ptr;
//     off_t i_blocks_ptr;
//     off_t d_blocks_ptr;
//     int raid_mode;
//     // Extend after this line
// };

// void create_fs(struct disk_info *disk, char* disk_name, int inode_ct, int dblock_ct, int raid)
// {

//     strncpy(disk->disk_name, disk_name, MAX_NAME);
//     disk->disk_name[MAX_NAME - 1] = '\0';

//     disk->sb.num_inodes = inode_ct;
//     disk->sb.num_data_blocks = dblock_ct;
//     disk->sb.raid_mode = raid;
//     disk->sb.i_bitmap_ptr = sizeof(struct wfs_sb);
//     disk->sb.d_bitmap_ptr = disk->sb.i_bitmap_ptr + (inode_ct;



//     return;
// }

void print_byte_binary(unsigned char byte) {
    printf("Byte in binary: \n");
    for (int i = 7; i >= 0; i--) {
        printf("%d", (byte >> i) & 1);
    }
    printf("\n");
}

void print_inode_deets(struct wfs_inode *inode)
{
    printf("Inode details : \n");
    printf("num = %d\n", inode->num);
    printf("mode = %d\n", inode->mode);
    printf("size = %ld\n", inode->size);
    printf("uid = %d\n", inode->uid);
    printf("gid = %d\n", inode->gid);
    printf("nlinks = %d\n", inode->nlinks);
    printf("atim = %ld\n", inode->atim);
    printf("atim = %ld\n", inode->mtim);
    printf("atim = %ld\n", inode->ctim);

}

void print_superblock_deets(struct wfs_sb *sb)
{
    if(sb == 0){
        printf("sb is null\n");
        return;
    }
    printf("num_inodes = %ld\n", sb->num_inodes);
    printf("num_data_blocks = %ld\n", sb->num_data_blocks);
    printf("i_bitmap_ptr = %ld\n", sb->i_bitmap_ptr);
    printf("d_bitmap_ptr = %ld\n", sb->d_bitmap_ptr);
    printf("i_blocks_ptr = %ld\n", sb->i_blocks_ptr);
    printf("d_blocks_ptr = %ld\n", sb->d_blocks_ptr);
    printf("raid = %d\n", sb->raid);
    printf("disk_ct = %d\n", sb->disk_ct);
    printf("disk_id = %d\n", sb->disk_id);
}

int create_and_initialize_disk(const char* disk_path, size_t inode_ct, size_t dblock_ct, int raid, int disk_id, int disk_ct) {
    
    // Create the file with read/write permissions
    int disk_fd = open(disk_path, O_RDWR, 0644);
    struct stat file_stat;
    if (stat(disk_path, &file_stat) != 0) {
        printf("Error getting file size\n");
        return 0;
    }

    // printf("Actual File size = %ld\n", file_stat.st_size);

    if (disk_fd == -1) {
        perror("Error creating disk file");
        return 0;
    }

    // Create superblock structure
    struct wfs_sb sb = {0};  // Initialize to zero
    
    // Set superblock parameters
    sb.num_inodes = inode_ct;
    sb.num_data_blocks = dblock_ct;
    sb.raid = raid;
    
    // Calculate sequential pointers
    sb.i_bitmap_ptr = sizeof(struct wfs_sb);
    
    // Inode bitmap (rounded up to bytes)
    sb.d_bitmap_ptr = sb.i_bitmap_ptr + ((inode_ct + 7) / 8);
    
    // Inode blocks
    sb.i_blocks_ptr = sb.d_bitmap_ptr + ((dblock_ct + 7) / 8);

    sb.i_blocks_ptr = sb.i_blocks_ptr % 512 == 0 ? sb.i_blocks_ptr : (sb.i_blocks_ptr/512 + 1)*512; 
    
    // Data blocks
    // Not checking for block alignment because i_block_ptr is already block aligned
    //
    sb.d_blocks_ptr = sb.i_blocks_ptr + (inode_ct * BLOCK_SIZE);  

    sb.d_blocks_ptr = sb.d_blocks_ptr % 512 == 0 ? sb.d_blocks_ptr : (sb.d_blocks_ptr/512 + 1)*512; 

    off_t total_size = sb.d_blocks_ptr + (sb.num_data_blocks * BLOCK_SIZE);  

    if(total_size > file_stat.st_size){
        printf("Requested file size greater than disk\n");
        return 255;
    }


    sb.disk_ct = disk_ct;
    sb.disk_id = disk_id;
    sb.disk_size = total_size;
    print_superblock_deets(&sb);

    // Calculate total file size

    // Write superblock to the beginning of the file
    ssize_t bytes_written = write(disk_fd, &sb, sizeof(struct wfs_sb));
    if (bytes_written != sizeof(struct wfs_sb)) {
        printf("Error writing superblock\n");
        close(disk_fd);
        return 0;
    }

    // // Extend the file to the calculated total size
    // if (ftruncate(disk_fd, total_size) == -1) {
    //     perror("Error setting file size");
    //     close(disk_fd);
    //     return -1;
    // }

    // Optional: Zero out the rest of the file
    // off_t current_pos = lseek(disk_fd, sizeof(struct wfs_sb), SEEK_SET);
    // if (current_pos == -1) {
    //     perror("Error seeking in file");
    //     close(disk_fd);
    //     return -1;
    // }

    // // Write zeros to the rest of the file
    // char zero_buffer[4096];
    // memset(zero_buffer, 0, sizeof(zero_buffer));
    
    // off_t remaining = total_size - sizeof(struct wfs_sb);
    // while (remaining > 0) {
    //     size_t write_size = (remaining > sizeof(zero_buffer)) ? sizeof(zero_buffer) : remaining;
    //     ssize_t written = write(disk_fd, zero_buffer, write_size);
        
    //     if (written == -1) {
    //         perror("Error zeroing file");
    //         close(disk_fd);
    //         return -1;
    //     }
        
    //     remaining -= written;
    // }

    char base_inode_bitmap = {0};

    base_inode_bitmap |= (1);

    // printf("jpd Base inode bitmap\n");

    // for (int i = 7; i >= 0; i--) {
    //     printf("%d", (base_inode_bitmap & (1 << i)) ? 1 : 0);
    // }

    // printf("jpd i_bitmap_ptr %ld\n", sb.i_bitmap_ptr);
    off_t current_pos = lseek(disk_fd, sb.i_bitmap_ptr, SEEK_SET);
    if (current_pos == -1) {
        perror("Error seeking in file while writing inode_bitmap\n");
        close(disk_fd);
        return 0;
    }

    bytes_written = write(disk_fd, &base_inode_bitmap, 1);
    if (bytes_written != 1) {
        printf("Error writing base inode data\n");
        close(disk_fd);
        return 0;
    }


    struct wfs_inode base_inode;
    base_inode.num = 0U;
    base_inode.mode = S_IFDIR | 0755;
    base_inode.size = 0U;
    base_inode.uid = getuid();
    base_inode.gid = getgid();
    base_inode.nlinks = 2U;
    base_inode.atim = base_inode.mtim = base_inode.ctim = time(NULL);
    // base_inode.blocks = {0};
    // memset(base_inode.blocks, 0U, sizeof(base_inode.blocks));

    // print_inode_deets(&base_inode);

    // printf("i_blocks_ptr = %ld\n", sb.i_blocks_ptr);

    current_pos = lseek(disk_fd, sb.i_blocks_ptr, SEEK_SET);
    if (current_pos == -1) {
        perror("Error seeking in file");
        close(disk_fd);
        return 0;
    }

    bytes_written = write(disk_fd, &base_inode, sizeof(struct wfs_inode));
    if (bytes_written != sizeof(struct wfs_inode)) {
        printf("Error writing base inode data\n");
        close(disk_fd);
        return 0;
    }

    close(disk_fd);

    disk_fd = open(disk_path, O_RDWR, 0644);

    unsigned char i_bitmap;

    // printf("Reading bitmap from file\n");
    current_pos = lseek(disk_fd, sb.i_bitmap_ptr, SEEK_SET);
    int bytes_read = read(disk_fd, (void *) &i_bitmap, sizeof(unsigned char));
    if(bytes_read != sizeof(unsigned char)){
        printf("something is up. invalid bitmap read");
    }
    // print_byte_binary(i_bitmap);

    struct wfs_inode buf;

    current_pos = lseek(disk_fd, sb.i_blocks_ptr, SEEK_SET);
    bytes_read = read(disk_fd, (void *) &buf, sizeof(struct wfs_inode));
    if(bytes_read != sizeof(struct wfs_inode)){
        printf("something is up. invlalid data read");
    }
    // print_inode_deets(&buf);
    return 1;
}



int main(int argc, char* argv[])
{
    fs_init_params(&my_fs);

    if(!validate_arguments(argc, argv)){
        printf("Invalid arguments. Exiting...\n");
        exit(EXIT_FAILURE);
    }

    if(!parse_arguments(argc, argv)){
        printf("Error parsing arguments. Exiting...\n");
        exit(EXIT_FAILURE);
    }
    
    for(int i = 0; i < my_fs.disks_ct; i++){
        int res = create_and_initialize_disk(my_fs.disks[i], my_fs.inodes_ct, my_fs.dblocks_ct, my_fs.raid, i, my_fs.disks_ct);
        if(res == 0 || res == 255)
        {
            return res;
        }
    }

    return 0;

}