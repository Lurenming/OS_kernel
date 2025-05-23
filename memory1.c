#define _CRT_SECURE_NO_WARNINGS 1
#include "memory1.h"

// 全局变量定义
MemoryBlock memory[MEMORY_SIZE];
DiskBlock disk[DISK_SIZE];
PCB* processes[100];       // 最多100个进程
TLBEntry tlb[TLB_SIZE];    // 快表
int process_count = 0;

// 初始化内存、磁盘和快表
void initialize() {
    // 初始化内存
    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i].data = NULL;
        memory[i].value = 0;
        memory[i].pagenum = -1;
        memory[i].process_id = -1;
        memory[i].size = 0;
    }

    // 初始化磁盘
    for (int i = 0; i < DISK_SIZE; i++) {
        disk[i].value = 0;
        disk[i].pagenum = -1;
        disk[i].process_id = -1;
        memset(disk[i].data, 0, PAGE_SIZE);
    }

    // 初始化快表
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].valid = 0;
        tlb[i].pagenum = -1;
        tlb[i].frame = -1;
        tlb[i].process_id = -1;
        tlb[i].time_since_access = 0; // 新增初始化
    }
}

// 查找空闲磁盘块
int find_free_disk_block() {
    for (int i = 0; i < DISK_SIZE; i++) {
        if (!disk[i].value) {
            return i;
        }
    }
    return -1; // 没有空闲磁盘块
}

// 查找快表中是否有对应的页表项
int tlb_lookup(int pid, int pagenum) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].process_id == pid && tlb[i].pagenum == pagenum) {
            tlb[i].time_since_access = 0; // 命中时重置为0
            return tlb[i].frame; // 返回帧号
        }
    }
    return -1; // 未命中
}

// 更新快表(使用LRU算法)
// 参数：
//   pid - 进程ID
//   pagenum - 要更新的页号
//   frame - 对应的物理帧号
void tlb_update(int pid, int pagenum, int frame) {
    // 首先查找该页是否已在快表中
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].process_id == pid && tlb[i].pagenum == pagenum) {
            // 如果找到匹配项，更新帧号和最后访问时间
            tlb[i].frame = frame;
            tlb[i].time_since_access = 0; // 重置时间变量
            return;
        }
    }

    // 更新所有TLB项的时间变量
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid) {
            tlb[i].time_since_access++;
        }
    }

    // 查找要替换的TLB项（基于LRU）
    int oldest_time = 0;
    int replace_entry = -1;

    // 首先尝试找无效项
    for (int i = 0; i < TLB_SIZE; i++) {
        if (!tlb[i].valid) {
            replace_entry = i;
            break;
        }
    }

    // 如果没有无效项，找最久未访问的
    if (replace_entry == -1) {
        oldest_time = 0;
        for (int i = 0; i < TLB_SIZE; i++) {
            if (tlb[i].time_since_access > oldest_time) {
                oldest_time = tlb[i].time_since_access;
                replace_entry = i;
            }
        }
    }

    // 执行替换
    if (replace_entry != -1) {
        tlb[replace_entry].valid = 1;
        tlb[replace_entry].process_id = pid;
        tlb[replace_entry].pagenum = pagenum;
        tlb[replace_entry].frame = frame;
        tlb[replace_entry].time_since_access = 0; // 新项初始化为0

        printf("TLB replaced entry %d (PID %d Page %d)\n",
            replace_entry, pid, pagenum);
    }
}
void update_all_processes_time_vars(int exclude_pid, int exclude_pagenum) {
    for (int i = 0; i < process_count; i++) {
        PCB* pcb = processes[i];
        for (int j = 0; j < pcb->page_count; j++) {
            // 跳过排除的页面（当前正在访问的页面）
            if (pcb->pid == exclude_pid && j == exclude_pagenum) {
                continue;
            }
            // 只更新在内存中的页面（time_since_access != -1）
            if (pcb->page_table[j].status == 1 && pcb->page_table[j].time_since_access != -1) {
                pcb->page_table[j].time_since_access++;
            }
        }
    }
}
// 创建新进程
void create_process(int pid, int size) {
    PCB* pcb = (PCB*)malloc(sizeof(PCB));
    pcb->pid = pid;
    pcb->size = size;
    pcb->page_count = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    pcb->page_table = (PageTableEntry*)malloc(pcb->page_count * sizeof(PageTableEntry));

    // 初始化页表：所有页面初始不在内存
    for (int i = 0; i < pcb->page_count; i++) {
        pcb->page_table[i].frame = -1;      // 初始无物理帧
        pcb->page_table[i].status = 0;     // 不在内存
        pcb->page_table[i].referenced = 0;
        pcb->page_table[i].modified = 0;
        pcb->page_table[i].time_since_access = -1; // 初始化为-1
        // 为每个页面分配磁盘空间
        int disk_block = find_free_disk_block();
        if (disk_block == -1) {
            printf("Error: No free disk blocks for PID %d\n", pid);
            free(pcb->page_table);
            free(pcb);
            return;
        }

        // 初始化磁盘页面数据
        snprintf(disk[disk_block].data, PAGE_SIZE, "PID%d-Page%d-DiskData", pid, i);
        disk[disk_block].value = 1;
        disk[disk_block].pagenum = i;
        disk[disk_block].process_id = pid;

        pcb->page_table[i].disk_block = disk_block;
        printf("Allocated PID %d Page %d to disk block %d\n", pid, i, disk_block);
    }

    processes[process_count++] = pcb;
    printf("Process %d created: %d pages (all on disk)\n", pid, pcb->page_count);
}

/**
 * LRU页面置换算法实现
 * 该函数用于找出内存中最久未使用的页面进行置换
 *
 * @return int 返回被选中置换的物理帧号，如果没有可置换的帧则返回-1
 */
int lru_replace() {
    unsigned int max_time = 0;
    int replace_frame = -1;

    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i].value == 1) {
            int pid = memory[i].process_id;
            int pagenum = memory[i].pagenum;
            PCB* pcb = NULL;

            // 查找对应进程
            for (int j = 0; j < process_count; j++) {
                if (processes[j]->pid == pid) {
                    pcb = processes[j];
                    break;
                }
            }

            // 只考虑在内存中的页面（time_since_access != -1）
            if (pcb && pcb->page_table[pagenum].time_since_access != -1) {
                if (pcb->page_table[pagenum].time_since_access > max_time) {
                    max_time = pcb->page_table[pagenum].time_since_access;
                    replace_frame = i;
                }
            }
        }
    }

    return replace_frame;
}
/**
 * 处理内存访问请求
 */
void access_process(int pid, int logical_addr, int is_write) {
    int page_num = logical_addr / PAGE_SIZE;
    int offset = logical_addr % PAGE_SIZE;

    // 查找进程
    PCB* pcb = NULL;
    for (int i = 0; i < process_count; i++) {
        if (processes[i]->pid == pid) {
            pcb = processes[i];
            break;
        }
    }

    if (!pcb || page_num >= pcb->page_count) {
        printf("Error: Invalid access by process %d to address %d\n", pid, logical_addr);
        return;
    }

    // 先在快表中查找
    int frame = tlb_lookup(pid, page_num);
    if (frame != -1) {
        // 快表命中
        void* physical_addr = (char*)memory[frame].data + offset;
        // 更新所有进程在内存中的页面时间变量（排除当前页面）
        update_all_processes_time_vars(pid, page_num);

        // 重置当前页面的时间变量
        pcb->page_table[page_num].time_since_access = 0;
        pcb->page_table[page_num].referenced = 1;//命中后访问位置为1
        if (is_write) {
            pcb->page_table[page_num].modified = 1;//如果是写入数据将修改位也置为1
        }
        printf("PID %d: Page %d TLB HIT at frame %d (PhysAddr: %p) %s\n",
            pid, page_num, frame, physical_addr,
            is_write ? "(WRITE)" : "(READ)");
        return;
    }

    // 快表未命中，查找页表
    // 页面命中
    if (pcb->page_table[page_num].status == 1) {
        frame = pcb->page_table[page_num].frame;
        void* physical_addr = (char*)memory[frame].data + offset;
        // 更新所有进程在内存中的页面时间变量（排除当前页面）
        update_all_processes_time_vars(pid, page_num);

        // 重置当前页面的时间变量
        pcb->page_table[page_num].time_since_access = 0;
        pcb->page_table[page_num].referenced = 1;
        if (is_write) {
            pcb->page_table[page_num].modified = 1;
        }
        printf("PID %d: Page %d PAGE TABLE HIT at frame %d (PhysAddr: %p) %s\n",
            pid, page_num, frame, physical_addr,
            is_write ? "(WRITE)" : "(READ)");

        // 更新快表
        tlb_update(pid, page_num, frame);
    }
    // 缺页中断
    else {
        printf("PID %d: Page %d MISS (%s)\n", pid, page_num, is_write ? "WRITE" : "READ");

        // 确保页面在磁盘上
        if (pcb->page_table[page_num].disk_block == -1) {
            printf("Error: Page %d has no disk backing!\n", page_num);
            return;
        }

        // 查找空闲内存块或置换
        int free_frame = -1;
        for (int i = 0; i < MEMORY_SIZE; i++) {
            if (!memory[i].value) {
                free_frame = i;
                break;
            }
        }

        // 无空闲则执行LRU置换
        if (free_frame == -1) {
            free_frame = lru_replace();
            printf("LRU selected frame %d for replacement\n", free_frame);

            // 获取被置换的页面信息
            int old_pid = memory[free_frame].process_id;
            int old_pagenum = memory[free_frame].pagenum;
            PCB* old_pcb = NULL;

            // 查找原进程
            for (int i = 0; i < process_count; i++) {
                if (processes[i]->pid == old_pid) {
                    old_pcb = processes[i];
                    break;
                }
            }

            if (old_pcb) {
                // 输出被换出的页面信息
                printf("Swapping OUT: PID %d Page %d from frame %d\n",
                    old_pid, old_pagenum, free_frame);

                // 查找或分配磁盘块
                int old_disk_block = old_pcb->page_table[old_pagenum].disk_block;
                if (old_disk_block == -1) {
                    // 如果没有磁盘块，分配一个新的
                    old_disk_block = find_free_disk_block();
                    if (old_disk_block == -1) {
                        printf("Error: No free disk blocks available for swapping out\n");
                    }
                    else {
                        old_pcb->page_table[old_pagenum].disk_block = old_disk_block;
                    }
                }

                // 无论页面是否被修改过，都将其写回磁盘
                if (old_disk_block != -1) {
                    memcpy(disk[old_disk_block].data, memory[free_frame].data, PAGE_SIZE);
                    disk[old_disk_block].value = 1;
                    disk[old_disk_block].pagenum = old_pagenum;
                    disk[old_disk_block].process_id = old_pid;
                    printf("Writing back page to disk block %d (modified: %d)\n",
                        old_disk_block, old_pcb->page_table[old_pagenum].modified);
                }

                // 更新原页表项
                old_pcb->page_table[old_pagenum].status = 0;
                old_pcb->page_table[old_pagenum].frame = -1;
                old_pcb->page_table[old_pagenum].time_since_access = -1; // 新增：换出后设为-1
                // 从快表中移除该页表项
                for (int i = 0; i < TLB_SIZE; i++) {
                    if (tlb[i].valid && tlb[i].process_id == old_pid && tlb[i].pagenum == old_pagenum) {
                        tlb[i].valid = 0;
                        break;
                    }
                }
            }

            // 释放原内存
            free(memory[free_frame].data);
        }

        // 从磁盘加载数据到内存
        memory[free_frame].data = malloc(PAGE_SIZE);
        memcpy(memory[free_frame].data,
            disk[pcb->page_table[page_num].disk_block].data,
            PAGE_SIZE);
        // 更新所有进程在内存中的页面时间变量（包括新页面）
        update_all_processes_time_vars(-1, -1); // 不排除任何页面

        // 新调入的页面时间变量初始化为0
        pcb->page_table[page_num].time_since_access = 0;
        // 更新内存块信息
        memory[free_frame].value = 1;
        memory[free_frame].pagenum = page_num;
        memory[free_frame].process_id = pid;
        memory[free_frame].size = PAGE_SIZE;

        // 更新页表
        pcb->page_table[page_num].frame = free_frame;
        pcb->page_table[page_num].status = 1;
        pcb->page_table[page_num].referenced = 1;
        pcb->page_table[page_num].modified = is_write ? 1 : 0;

        // 更新快表
        tlb_update(pid, page_num, free_frame);

        printf("PID %d: Page %d loaded to frame %d (from disk block %d)\n",
            pid, page_num, free_frame, pcb->page_table[page_num].disk_block);
    }
}
//结束进程
void terminate_process(int pid) {
    int index = -1;
    PCB* pcb = NULL;

    // 查找进程
    for (int i = 0; i < process_count; i++) {
        if (processes[i]->pid == pid) {
            index = i;
            pcb = processes[i];
            break;
        }
    }

    if (!pcb) {
        printf("Error: Process %d not found\n", pid);
        return;
    }

    printf("Terminating process %d (total pages: %d):\n", pid, pcb->page_count);

    // 处理内存中的页面
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i].process_id == pid) {
            int page_num = memory[i].pagenum;

            if (pcb->page_table[page_num].modified) {
                // 处理脏页（写回磁盘）
                int disk_block = pcb->page_table[page_num].disk_block;
                if (disk_block == -1) {
                    disk_block = find_free_disk_block();
                    if (disk_block == -1) {
                        printf("  [Error] Can't save dirty page %d (no disk space)\n", page_num);
                    }
                    else {
                        pcb->page_table[page_num].disk_block = disk_block;
                        memcpy(disk[disk_block].data, memory[i].data, PAGE_SIZE);
                        disk[disk_block].value = 1;
                        disk[disk_block].pagenum = page_num;
                        disk[disk_block].process_id = pid;
                        printf("  [Writeback] Page %d -> Disk block %d\n", page_num, disk_block);
                    }
                }
                else {
                    // 已有磁盘块，直接覆盖
                    memcpy(disk[disk_block].data, memory[i].data, PAGE_SIZE);
                    printf("  [Update] Page %d -> Disk block %d (updated)\n", page_num, disk_block);
                }
            }
            else {
                // 干净页直接释放
                printf("  [Release] Page %d (clean)\n", page_num);
            }

            // 释放内存块
            free(memory[i].data);
            memory[i].data = NULL;
            memory[i].value = 0;
            memory[i].pagenum = -1;
            memory[i].process_id = -1;
            memory[i].size = 0;
        }
    }
    // 彻底清理TLB中该进程的所有项
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].process_id == pid) {
            tlb[i].valid = 0;
            tlb[i].pagenum = -1;
            tlb[i].frame = -1;
            tlb[i].process_id = -1;
            tlb[i].time_since_access = 0;
            printf("  [TLB Cleanup] Removed TLB entry %d (PID %d)\n", i, pid);
        }
    }

    // 释放PCB资源
    free(pcb->page_table);
    free(pcb);

    // 从进程数组中移除
    for (int i = index; i < process_count - 1; i++) {
        processes[i] = processes[i + 1];
    }
    process_count--;

    printf("Process %d terminated successfully\n", pid);
}

// 打印内存状态
void print_memory_status() {
    printf("\nMemory Status:\n");
    printf("Frame\tValue\tPage\tPID\n");
    for (int i = 0; i < MEMORY_SIZE; i++) {
        printf("%d\t%d\t%d\t%d\n", i, memory[i].value, memory[i].pagenum, memory[i].process_id);
    }
}

// 打印页表
void print_page_table(int pid) {
    PCB* pcb = NULL;
    for (int i = 0; i < process_count; i++) {
        if (processes[i]->pid == pid) {
            pcb = processes[i];
            break;
        }
    }

    if (!pcb) {
        printf("Process %d not found\n", pid);
        return;
    }

    printf("\nPage Table for PID %d:\n", pid);
    printf("Page\tFrame\tStatus\tTime\tModified\tDiskBlock\n");
    for (int i = 0; i < pcb->page_count; i++) {
        printf("%d\t%d\t%d\t%ld\t\t%d\t\t%d\n",
            i,
            pcb->page_table[i].frame,
            pcb->page_table[i].status,
            pcb->page_table[i].time_since_access,
            pcb->page_table[i].modified,
            pcb->page_table[i].disk_block);
    }
}

// 打印磁盘状态
void print_disk_status() {
    printf("\nDisk Status:\n");
    printf("Block\tValue\tPage\tPID\n");
    for (int i = 0; i < DISK_SIZE; i++) {
        if (disk[i].value) {
            printf("%d\t%d\t%d\t%d\n", i, disk[i].value, disk[i].pagenum, disk[i].process_id);
        }
    }
}

// 打印快表状态
void print_tlb_status() {
    printf("\nTLB Status:\n");
    printf("Entry\tValid\tPID\tPage\tFrame\tTime\n");
    for (int i = 0; i < TLB_SIZE; i++) {
        printf("%d\t%d\t%d\t%d\t%d\t%ld\n",
            i,
            tlb[i].valid,
            tlb[i].process_id,
            tlb[i].pagenum,
            tlb[i].frame,
            tlb[i].time_since_access);
    }
}


int main() {
    initialize();
    printf("Virtual Memory Management System\n");
    printf("Commands:\n");
    printf("1 - Create process (PID size)\n");
    printf("2 - Access page (w/r PID page_num)\n");
    printf("3 - Terminate process (PID)\n");
    printf("4 - Print status\n");
    printf("q - Quit\n\n");

    char command[10];
    while (1) {
        printf("> ");
        if (scanf("%s", command) != 1) {
            break;
        }

        if (strcmp(command, "q") == 0) {
            break;
        }
        else if (strcmp(command, "1") == 0) {
            // 创建进程
            printf("Enter PID and size (or 'c' to cancel): ");
            char input[20];
            scanf("%s", input);
            if (strcmp(input, "c") == 0) {
                continue;
            }
            int pid = atoi(input);
            int size;
            scanf("%d", &size);
            create_process(pid, size);
        }
        else if (strcmp(command, "2") == 0) {
            // 访问页面
            printf("Enter access type (w/r), PID and page number (or 'c' to cancel): ");
            char input[20];
            scanf("%s", input);
            if (strcmp(input, "c") == 0) {
                continue;
            }
            char access_type = input[0];
            int pid, page_num;
            scanf("%d %d", &pid, &page_num);
            int is_write = (access_type == 'w' || access_type == 'W') ? 1 : 0;
            access_process(pid, page_num * PAGE_SIZE, is_write);
        }
        else if (strcmp(command, "3") == 0) {
            // 终止进程
            printf("Enter PID to terminate (or 'c' to cancel): ");
            char input[20];
            scanf("%s", input);
            if (strcmp(input, "c") == 0) {
                continue;
            }
            int pid = atoi(input);
            terminate_process(pid);
        }
        else if (strcmp(command, "4") == 0) {
            // 打印状态
            printf("\n==== Current Status ====\n");
            for (int i = 0; i < process_count; i++) {
                print_page_table(processes[i]->pid);
            }
            print_memory_status();
            print_disk_status();
            print_tlb_status();
        }
        else {
            printf("Invalid command. Try again.\n");
        }
    }

    // 退出前终止所有进程
    while (process_count > 0) {
        terminate_process(processes[0]->pid);
    }

    return 0;
}