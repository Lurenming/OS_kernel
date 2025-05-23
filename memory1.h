#pragma once
#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <string.h>
#include <time.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE 4096       // 页面大小为4KB
#define MEMORY_SIZE 4        // 内存块数量
#define DISK_SIZE 256        // 磁盘块数量
#define TLB_SIZE 4           // 快表大小

// 快表项结构
typedef struct {
    int pagenum;            // 页号
    int frame;              // 页帧(内存块号)
    int process_id;         // 进程ID
    int valid;              // 有效位(0无效，1有效)
    unsigned int time_since_access; // 记录TLB项未被访问的时间
} TLBEntry;

// 页表项结构
typedef struct {
    int frame;              // 页帧(内存块号)
    int status;             // 状态位(0未调入，1已调入)
    int referenced;         // 访问位(0未访问，1已访问)
    int modified;           // 修改位(0未修改，1已修改)
    int disk_block;         // 磁盘块号(-1表示不在磁盘)
    unsigned int time_since_access; // 记录页面未被访问的时间
} PageTableEntry;

typedef struct {
    void* data;             // 指向实际分配的内存区域
    int value;              // 标记是否已分配
    int pagenum;            // 页号
    int process_id;         // 占用该内存块的进程ID
    size_t size;            // 分配的内存大小
} MemoryBlock;

// 磁盘块结构
typedef struct {
    int value;              // 0为空，1为存在数据
    int pagenum;            // 页号
    int process_id;         // 占用该磁盘块的进程ID
    char data[PAGE_SIZE];   // 页面数据内容
} DiskBlock;

// 进程控制块结构
typedef struct {
    int pid;                // 进程ID
    int size;               // 进程内存大小(字节)
    int page_count;         // 进程需要的页面数量
    PageTableEntry* page_table; // 页表指针
} PCB;

// 全局变量声明
extern MemoryBlock memory[MEMORY_SIZE];
extern DiskBlock disk[DISK_SIZE];
extern PCB* processes[100];
extern TLBEntry tlb[TLB_SIZE];
extern int process_count;

// 函数声明
void initialize();
int find_free_disk_block();
int tlb_lookup(int pid, int pagenum);
void tlb_update(int pid, int pagenum, int frame);
void create_process(int pid, int size);
int lru_replace();
void access_process(int pid, int logical_addr, int is_write);
void terminate_process(int pid);
void print_memory_status();
void print_page_table(int pid);
void print_disk_status();
void print_tlb_status();

#endif // MEMORY_MANAGER_H