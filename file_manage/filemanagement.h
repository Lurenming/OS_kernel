#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>


#define BLOCK_SIZE 64          // ���ݿ�64�ֽ�
#define MAX_FILE_SIZE 256      // ����ļ�����
#define MAX_FILE_NUM 256       // ����ļ�����
#define MAX_BLOCK 1024         // ���̿�����
#define INODE_START 1          // inode����ʼ��� 
#define BITMAP_START 129       // bitmap����ʼ���
#define DATA_START (BITMAP_START + (sizeof(char)*MAX_BLOCK)/BLOCK_SIZE) // data����ʼ���(145)
#define DEV_NAME "disk.bin"

#define Name_length 8         // �ļ�������󳤶�
#define iNode_NUM 256          // iNode������
#define DIR_NUM 5             // ÿ��Ŀ¼�ļ��µ�����ļ�����
#define PATH_LENGTH 100        // ·���ַ�����󳤶�
#define FBLK_NUM 4             // ÿ���ļ�iNode�б���Ŀ����

#define RDONLY 00 //ֻ��
#define WRONLY 01 //ֻд
#define RDWR 02   //��д

//----------------���̽ṹ����----------------

// ������ṹ��
typedef struct super_block {
    unsigned short inodeNum;    // ������inode�ڵ���
    unsigned short blockNum;    // �����е��������
    unsigned long file_maxsize; // �ļ�����󳤶�
} super_block;

// iNode�ṹ�壨32�ֽڣ�
typedef struct INODE {
    int i_mode;               // �ļ����ͣ�0Ŀ¼��1��ͨ�ļ�
    int i_size;               // �ļ���С���ֽ�����
    int permission;           // �ļ��Ķ���д��ִ��Ȩ��
    int ctime;                // �ļ���ʱ�������δ����
    int mtime;                // �ļ���ʱ�������δ����
    int nlinks;               // ���������ж���Ŀ¼��ָ���inode��
    int block_address[FBLK_NUM]; // �ļ����ݿ�ĵ�ַ����
    int open_num;             // �򿪼�����0��ʾδ�򿪣�1��ʾ�Ѵ�
} iNode;

// Ŀ¼��ṹ��
typedef struct dir_entry {
    char name[Name_length];   // �ļ���
    int inode;                // ������inode���
} dir_entry;

// Ŀ¼�ṹ��
typedef struct directory {
    int num_entries;          // ��ǰĿ¼�µ�Ŀ¼������
    dir_entry entries[DIR_NUM]; // Ŀ¼������
} directory;

//�ļ����
typedef struct OS_FILE {
    iNode* f_iNode;    // ָ���Ӧ��iNode
    long f_pos;        // ��ǰ�ļ�ָ��λ��
    int f_mode;        // ��ģʽ (RDONLY/WRONLY/RDWR)
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
int find_free_inode();//�ҵ�һ������inode

int resolve_path(const char* path, int* inode_num_out);
int create_entry(const char* path, int is_dir, int permission);
int delete_entry(const char* path);
int add_dir_entry(int parent_inode, const char* name, int new_inode);

void dir_ls (inode_num);

// �ļ��������
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