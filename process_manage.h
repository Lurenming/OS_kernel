#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_PROCESSES 10 // 可调入CPU的最大进程数
#define CPU_TIME_SLOT 1  // CPU时间片
#define RR_TIME_SLOT 1  // RR方式的时间片长度，需要是CPU时间片长度的整数倍

typedef struct PCB {
    int pid;                 // 进程编号
    char process_name[10];    // 进程名
    char state[10];           // 进程状态（就绪、运行、阻塞）
    int start_time_slot;      // 进程开始运行的时间片
    int process_time_slot;    // 进程时间片
    int priority;             // 优先级,低数值代表高优先级,取值为1-5
    struct PCB *next;         // 指向下一个进程的指针
} PCB;

int process_count = 0; // CPU中进程总数
int timer = 0; //计数器，打印程序执行信息
int scheNum = 0; // 算法序号
int mode = 0; // mode=0代表非抢占调度算法，=1代表抢占性

typedef struct QUEUE {
    char queue_name[10]; // 队列名称
    PCB *head;           // 队列头
    PCB *tail;           // 队列尾，方便插入
    int pcb_count;       // 队列进程数
} QUEUE;

typedef struct LOCK{
    char file_addr[50]; // 共享文件地址
    char lockmode; // 锁类型(w写,r读)
    int pid; // 目前占用该锁的进程id
    struct LOCK *next; // 指向下一个锁的指针
} LOCK;

typedef struct LOCK_POOL{
    LOCK *lock_list;
    int lock_count; // 锁池中锁的数量
} LOCK_POOL;
LOCK_POOL lock_buffer; // 待处理的要求锁的序列
LOCK_POOL lock_in_use; // 锁池，已经获得锁的序列
LOCK_POOL* p_lock_buffer =&lock_buffer; //两个锁池的指针，便于后续调用
LOCK_POOL* p_lock_in_use =&lock_in_use;

QUEUE start_queue;  //开始状态的队列
QUEUE ready_queue;  //就绪队列
QUEUE running_queue;  //运行队列
QUEUE* p_start_queue = &start_queue;
QUEUE blocked_queue[3];  //阻塞队列，阻塞原因可能有多种,queue[0]表示文件锁被占用，queue[1]表示IO等待，queue[2]表示缺页
QUEUE* p_ready_queue = &ready_queue;    //三个队列的指针，便于后续调用
QUEUE* p_running_queue = &running_queue;
QUEUE* p_blocked_queue0 = &blocked_queue[0]; // 阻塞原因0
QUEUE* p_blocked_queue1 = &blocked_queue[1]; // 阻塞原因1

void initqueue();
void init_lock_pool();
PCB* create_process(int pid, int priority, char process_name[10], int start_time_slot, int time_slot);
void add_to_queue(QUEUE *queue, PCB *process);
void remove_from_queue(QUEUE *queue, PCB *process);
PCB* priority_dispatch();
PCB* check_preemption(PCB* process_in_execute);
void add_to_lock_list(LOCK* newlock, LOCK_POOL *tempool);
LOCK* remove_from_lock_list(PCB* process,char lockmode,char file_addr[],LOCK_POOL *tempool);
void check_blocked_process_lock();
void enter_critical_section(PCB* process,char lockmode,char file_addr[]);
void leave_critical_section(PCB* process,char lockmode,char file_addr[]);
void execute_process();
void run();
void print_process(PCB* process);
void print_queue(QUEUE* queue);
