#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include "helpers.c"
// #include "wfs.h"

// Info of all the cmd params 
//
struct {
    char disks[MAX_DISKS][MAX_NAME];
    char mount_path[MAX_NAME];
    char **fuse_args;
    int disks_ct;
    int fuse_args_ct;
    int raid;
} wfs_params;

char *disk_mmaps[MAX_DISKS];

// Display all cmd params
//
void print_wfs_params(){
    printf("+-------------------------------------------+\n");
    printf("|              WFS Parameters               |\n");
    printf("+-------------------------------------------+\n");
    printf("WFS params: \n");
    for(int i = 0; i < wfs_params.disks_ct; i++){
        printf("Disk %d: %s\n", i, wfs_params.disks[i]);
    }
    printf("Mount path: %s\n", wfs_params.mount_path);

    printf("Fuse flags: ");
    for(int i = 0; i < wfs_params.fuse_args_ct; i++){
        printf("%s ", wfs_params.fuse_args[i]);
    }
    printf("\n+-------------------------------------------+\n");
}

// Cleanup cmd params
//
void free_wfs_params(){

    for(int i = 0; i < wfs_params.fuse_args_ct; i++){
        if(wfs_params.fuse_args[i] != NULL)
            free(wfs_params.fuse_args[i]);
    }

    if(wfs_params.fuse_args != NULL)
        free(wfs_params.fuse_args);
}

// Free up any dynamically allocated memory
//
void cleanup(){
    free_wfs_params();
}


void cleanup_and_exit(int status)
{
    cleanup();
    exit(status);
}

// Checks if a path is valid and if it is, it populates the stat struct
//
int is_valid_path(const char *path, struct stat *sb){

    if(stat(path, sb) == 0){
        return OP_SUCCESS;
    }

    return OP_FAIL;
}

/// @brief Verify if the disk images are valid and if the disks are okay to be
/// mounted.
/// @return 
///     OP_SUCCESS if they are valid
///     OP_FAIL if they are invalid
///
int validate_disks(){
    

    for(int i = 0; i < wfs_params.disks_ct; i++)
    {
        struct stat ds;

        // If not a valid file, return error
        //
        if(is_valid_path(wfs_params.disks[i], &ds) != OP_SUCCESS || !S_ISREG(ds.st_mode))
        {
            // perror("%s - Invalid disk path. Please check and try again\n", wfs_params.disks[i]);
            fprintf(stderr,"Invalid disk path. Please check and try again\n");
            return OP_FAIL;
        }
    }

    int disk_bitmap[wfs_params.disks_ct];
    
    int disks_ct = -1;

    // 
    for(int i = 0; i < wfs_params.disks_ct; i++)
    {
        int fd = open(wfs_params.disks[i], O_RDONLY);

        if (fd == -1) 
        {
            fprintf(stderr,"Failed to open file %s: %s\n", wfs_params.disks[i], strerror(errno));
            return -1;
        }

        struct wfs_sb sb;

        ssize_t bytes_read = read(fd, &sb, sizeof(sb));
    
        if (bytes_read != sizeof(sb)) {
            close(fd);
            fprintf(stderr,"Failed to read %zu bytes: %s\n", sizeof(sb), strerror(errno));
            return OP_FAIL;
        }

        if(disks_ct == -1)
        {
            disks_ct = sb.disk_ct;
        }
        
        if(disks_ct != sb.disk_ct)
        {
            fprintf(stderr, "Mismatch in disks count in the disk images. Are you sure you've mentioned the right disks\n");
            return OP_FAIL;
        }

        disk_bitmap[sb.disk_id] = 1;

        close(fd);
    }
    
    if(disks_ct != wfs_params.disks_ct)
    {
        fprintf(stderr, "Disk image disks count and number of disks passed to cmd are different\n");
        return OP_FAIL;
    }

    for(int i = 0; i < disks_ct; i++){
        if(disk_bitmap[i] != 1)
        {
            fprintf(stderr,"Invalid set of disks present\n");
            return OP_FAIL;
        }
    }

    return OP_SUCCESS;
}


/// @brief mmap all disks so they're easily accessible
/// @return 
///     OP_SUCCESS if all mmaps were created successfully
///     OP_FAIL if mmap was not created successfully for a disk
///
int mmap_disks()
{
    for(int i = 0; i < wfs_params.disks_ct; i++)
    {
        int fd = open(wfs_params.disks[i], O_RDWR, 0644);
        
        if (fd == -1) {
            fprintf(stderr, "Failed to open disk - %s\n", wfs_params.disks[i]);
            return OP_FAIL;
        }

        // Get file size
        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            fprintf(stderr,"Failed to get file size - %s\n", wfs_params.disks[i]);
            close(fd);
            return OP_FAIL;
        }

        // Memory map the file
        //
        disk_mmaps[i] = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (disk_mmaps[i] == MAP_FAILED) {
            fprintf(stderr, "Memory mapping failed for disk - %s\n", wfs_params.disks[i]);
            close(fd);
            return OP_FAIL;
        }
    }

    return OP_SUCCESS;
}




// static int wfs_getattr(const char *path, struct stat *stbuf)
// {
// 	int res;

// 	res = lstat(path, stbuf);
// 	if (res == -1)
// 		return -errno;

// 	return 0;
// }

// static int wfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
// 		       off_t offset, struct fuse_file_info *fi)
// {
// 	DIR *dp;
// 	struct dirent *de;

// 	(void) offset;
// 	(void) fi;

// 	dp = opendir(path);
// 	if (dp == NULL)
// 		return -errno;

// 	while ((de = readdir(dp)) != NULL) {
// 		struct stat st;
// 		memset(&st, 0, sizeof(st));
// 		st.st_ino = de->d_ino;
// 		st.st_mode = de->d_type << 12;
// 		if (filler(buf, de->d_name, &st, 0))
// 			break;
// 	}

// 	closedir(dp);
// 	return 0;
// }

// static int wfs_mknod(const char *path, mode_t mode, dev_t rdev)
// {
// 	int res;

// 	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
// 	   is more portable */
// 	if (S_ISREG(mode)) {
// 		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
// 		if (res >= 0)
// 			res = close(res);
// 	} else if (S_ISFIFO(mode))
// 		res = mkfifo(path, mode);
// 	else
// 		res = mknod(path, mode, rdev);
// 	if (res == -1)
// 		return -errno;

// 	return 0;
// }

// static int wfs_mkdir(const char *path, mode_t mode)
// {
//     char *path_cpy = strdup(path);

//     char *path_token = strtok(path, "/");
//     while(path_token != NULL){
//         printf("%s ", path_token);


//         path_token = strtok(NULL, "/");
//     }
//     for()

//     res = mkdir(path, mode);
//     if (res == -1)
//         return -errno;

//     return 0;

//     if()

// }

// static int wfs_unlink(const char *path)
// {
// 	int res;

// 	res = unlink(path);
// 	if (res == -1)
// 		return -errno;

// 	return 0;
// }

// static int wfs_rmdir(const char *path)
// {
// 	int res;

// 	res = rmdir(path);
// 	if (res == -1)
// 		return -errno;

// 	return 0;
// }

// static int wfs_read(const char *path, char *buf, size_t size, off_t offset,
// 		    struct fuse_file_info *fi)
// {
// 	int fd;
// 	int res;

// 	(void) fi;
// 	fd = open(path, O_RDONLY);
// 	if (fd == -1)
// 		return -errno;

// 	res = pread(fd, buf, size, offset);
// 	if (res == -1)
// 		res = -errno;

// 	close(fd);
// 	return res;
// }

// static int wfs_write(const char *path, const char *buf, size_t size,
// 		     off_t offset, struct fuse_file_info *fi)
// {
// 	int fd;
// 	int res;

// 	(void) fi;
// 	fd = open(path, O_WRONLY);
// 	if (fd == -1)
// 		return -errno;

// 	res = pwrite(fd, buf, size, offset);
// 	if (res == -1)
// 		res = -errno;

// 	close(fd);
// 	return res;
// }

// static struct fuse_operations ops = {
//   // .getattr = wfs_getattr,
//   // .mknod   = wfs_mknod,
//   .mkdir   = wfs_mkdir,
//   // .unlink  = wfs_unlink,
//   // .rmdir   = wfs_rmdir,
//   // .read    = wfs_read,
//   // .write   = wfs_write,
//   // .readdir = wfs_readdir,
// };

int parse_args(int argc, char *argv[]){
    
    if(argc < 3){
        fprintf(stderr,"Invalid number of arguments. Exiting... \n"); 
        return OP_FAIL;
    }

    int is_fuse_arg = 0, fuse_args_ct = 0, fuse_args_start_index = 0;
    int disks_ct = 0;
    
    // Order of args 
    // 1. Executable
    // 2. Disks 
    // 3. Fuse flags
    // 4. Mount path
    //
    for(int i = 1; i < argc; i++){

        // Disks path
        if(argv[i][0] != '-' && is_fuse_arg == 0)
        {
            int len = (int)strlen(argv[i]); 
            memcpy(wfs_params.disks[disks_ct], argv[i],len);
            wfs_params.disks[disks_ct][len] = '\0';

            disks_ct++;
        }
        else if(argv[i][0] == '-' || i != argc - 1) // last arg is mount_path
        {
            if(is_fuse_arg == 0){
                is_fuse_arg = 1;
                fuse_args_start_index = i;
            }
            fuse_args_ct++;
        }
        else
        {
            int len = (int) strlen(argv[i]);
            memcpy(wfs_params.mount_path, argv[i],len);
            wfs_params.mount_path[len] = '\0';
        }
    }

    wfs_params.disks_ct = disks_ct;
    wfs_params.fuse_args_ct = fuse_args_ct;

    wfs_params.fuse_args = malloc(fuse_args_ct * sizeof(char *));
    for(int i = 0; i < fuse_args_ct; i++)
    {
        wfs_params.fuse_args[i] =  strdup(argv[fuse_args_start_index + i]);

        if(wfs_params.fuse_args[i] == 0)
        {
            cleanup();
            fprintf(stderr,"Error duplicating fuse arguments. Exiting...\n");
            return SYS_ERROR;
        }
    }
    return OP_SUCCESS;
}


int main(int argc, char *argv[])
{
    umask(0);

    if(parse_args(argc, argv) == OP_FAIL){
        fprintf(stderr,"Verify arguments and try again!\n"); 
        exit(EXIT_FAILURE);
    }

    print_wfs_params();

    if(validate_disks() != OP_SUCCESS)
    {
        fprintf(stderr, "Disks validation failed\n");
        cleanup_and_exit(EXIT_FAILURE);
    }

    // mmap disks
    //
    if(mmap_disks() != OP_SUCCESS)
    {
        fprintf(stderr, "Failed to mmap disks\n");
        cleanup_and_exit(EXIT_FAILURE);
    }

    for(int i = 0; i < wfs_params.disks_ct; i++){
        struct wfs_sb *sb = (struct wfs_sb *)disk_mmaps[i];

        print_superblock_deets(sb);
        get_inode(disk_mmaps[i], 5);
        DEBUG_PRINT("Size of sb: %ld\n", sizeof(*sb));
        int inode = 0;
        char *i_bitmap_ptr = get_ibitmap(disk_mmaps[i]);
        DEBUG_PRINT("Is index %d bit set %d\n", 0, is_bit_set(i_bitmap_ptr, inode));

        DEBUG_PRINT("Free inode slot at: %d\n", find_free_inode(disk_mmaps[i]));
        inode = 12;
        DEBUG_PRINT("Is index %d bit set %d\n", inode, is_bit_set(i_bitmap_ptr, inode));
        set_bit(i_bitmap_ptr, inode, 1);
        DEBUG_PRINT("Is index %d bit set %d\n", inode, is_bit_set(i_bitmap_ptr, inode));
        DEBUG_PRINT("Free inode slot at: %d\n", find_free_inode(disk_mmaps[i]));
        set_bit(i_bitmap_ptr, inode, 0);
        DEBUG_PRINT("Is index %d bit set %d\n", inode, is_bit_set(i_bitmap_ptr, inode));
        DEBUG_PRINT("Free inode slot at: %d\n", find_free_inode(disk_mmaps[i]));
    }

    // return fuse_main(argc, argv, &ops, NULL);

    cleanup_and_exit(EXIT_SUCCESS);
}
