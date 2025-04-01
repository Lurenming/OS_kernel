#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>


#define BLOCK_SIZE 64          // 数据块64字节
#define MAX_FILE_SIZE 256      // 最大文件长度
#define MAX_FILE_NUM 256       // 最大文件数量
#define MAX_BLOCK 1024         // 磁盘块总数
#define INODE_START 1          // inode区起始块号 
#define BITMAP_START 129       // bitmap区起始块号
#define DATA_START (BITMAP_START + (sizeof(char)*MAX_BLOCK)/BLOCK_SIZE) // data区起始块号(145)
#define DEV_NAME "disk.bin"

#define Name_length 8         // 文件名称最大长度
#define iNode_NUM 256          // iNode的数量
#define DIR_NUM 5             // 每个目录文件下的最大文件个数
#define PATH_LENGTH 100        // 路径字符串最大长度
#define FBLK_NUM 4             // 每个文件iNode中保存的块号数

#define RDONLY 00 //只读
#define WRONLY 01 //只写
#define RDWR 02   //读写

//----------------磁盘结构定义----------------

// 超级块结构体
typedef struct super_block {
    unsigned short inodeNum;    // 磁盘中inode节点数
    unsigned short blockNum;    // 磁盘中的物理块数
    unsigned long file_maxsize; // 文件的最大长度
} super_block;

// iNode结构体（32字节）
typedef struct INODE {
    int i_mode;               // 文件类型：0目录，1普通文件
    int i_size;               // 文件大小（字节数）
    int permission;           // 文件的读、写、执行权限
    int ctime;                // 文件的时间戳（暂未处理）
    int mtime;                // 文件的时间戳（暂未处理）
    int nlinks;               // 链接数（有多少目录项指向该inode）
    int block_address[FBLK_NUM]; // 文件数据块的地址数组
    int open_num;             // 打开计数，0表示未打开，1表示已打开
} iNode;

// 目录项结构体
typedef struct dir_entry {
    char name[Name_length];   // 文件名
    int inode;                // 关联的inode编号
} dir_entry;

// 目录结构体
typedef struct directory {
    int num_entries;          // 当前目录下的目录项数量
    dir_entry entries[DIR_NUM]; // 目录项数组
} directory;

//文件句柄
typedef struct OS_FILE {
    iNode* f_iNode;    // 指向对应的iNode
    long f_pos;        // 当前文件指针位置
    int f_mode;        // 打开模式 (RDONLY/WRONLY/RDWR)
    int f_inode_num;
} OS_FILE;

bool disk_format();
bool disk_init();
bool disk_activate();
bool block_write(long block, char* buf);
bool block_read(long block, char* buf);
int alloc_first_free_block();
int free_allocated_block(int block_num);
int get_inode(int inode_num, iNode* inode);
int find_free_inode();//找到一个空闲inode

int resolve_path(const char* path, int* inode_num_out);
int create_entry(const char* path, int is_dir, int permission);
int delete_entry(const char* path);
int add_dir_entry(int parent_inode, const char* name, int new_inode);

void dir_ls (inode_num);

// 文件句柄操作
OS_FILE* Open_File(const char* path, int mode);
int file_write(OS_FILE* f, const char* data, int len);
int file_read(OS_FILE* f, char* buf, int len);
void Close_File(OS_FILE* f);

void handle_mkdir(const char* path);
void handle_touch(const char* path);
void handle_rm(const char* path);
void handle_ls(const char* path);
void handle_write(const char* path, const char* data);
void handle_read(const char* path);
void print_help();