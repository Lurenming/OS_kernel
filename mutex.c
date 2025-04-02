# include "process_manage.h"

typedef struct LOCK{
    // 进程锁数据结构定义，用于链表
    char shared_file_addr[50]; // 共享文件地址
    char lockmode; // 锁类型(w写,r读)
    int pid; // 目前占用该锁的进程id
    struct LOCK *next; // 指向下一个锁的指针
} LOCK;

typedef struct LOCK_POOL{
    // 锁池数据类型
    LOCK *lock_list;
    int lock_count; // 锁池中锁的数量
} LOCK_POOL;
LOCK_POOL lock_pool; // 锁池

void init_lock_pool() //初始化锁池
{
    lock_pool.lock_list = (LOCK *)malloc(sizeof(LOCK));
    lock_pool.lock_list->next = NULL;
    lock_pool.lock_count = 0;
}

void enter_critical_section(PCB* process,char lockmode,char file_addr[])  
{
    //进入临界区部分的代码
    //mode 代表进程的对共享文件的操作模式，分为w(写)和r(读)
    int i=0;
    int count=lock_pool.lock_count;
    LOCK* lock_list = lock_pool.lock_list;
    for(; i< count; i++) //遍历锁池，查找是否有进程占有该文件的锁
    {
        if(strcmp(lock_list->shared_file_addr,file_addr) == 0) //该文件已经有进程占有锁
        {
            if(lock_list->pid == process->pid) //占有该锁的是该进程，则将lockmode更新为mode（可能是由读变为写或相反）
            {   
                lock_list->lockmode = lockmode;
                return;
            }
            else //占有该锁的不是该进程,而是其他进程。则根据mode的不同，进行不同的操作
            {
                if(lock_list->lockmode == 'w') // 其他进程以写模式占有锁
                {   
                    //block process 阻塞该进程，直到该文件的锁被释放
                    return;
                }
                else // 其他进程进程以读模式占有锁
                {
                    if (lockmode== 'r') //如果该进程也是要求读，那么可以共享锁,在锁池末尾添加锁
                    {
                        while(lock_pool.lock_list->next != NULL)
                            lock_pool.lock_list = lock_pool.lock_list->next;
                        LOCK *newlock = (LOCK *)malloc(sizeof(LOCK));
                        newlock->lockmode = lockmode;
                        newlock->pid = process->pid;
                        strcpy(newlock->shared_file_addr,file_addr);
                        newlock->next = NULL;
                        lock_pool.lock_list->next = newlock;
                        lock_pool.lock_count++;
                        return;
                    }
                    else // 该进程要求写，那么不允许共享锁，阻塞该进程
                    {
                        //block process 阻塞该进程，直到该文件的锁被释放
                        return;
                    }
                }   
            }
            break;
        }
        lock_list = lock_list->next;
    }
    if(i == count) //遍历结束，没有任何进程占有锁,则直接在锁池末尾分配锁给该进程
    {
        LOCK *newlock = (LOCK *)malloc(sizeof(LOCK));
        newlock->lockmode = lockmode;
        newlock->pid = process->pid;
        strcpy(newlock->shared_file_addr,file_addr);
        newlock->next = NULL;
        if(lock_pool.lock_count == 0) //锁池为空,将首个锁赋对应的值
        {
            lock_pool.lock_list = newlock;
            lock_pool.lock_count++;
            return;
        }
        else //否则遍历到锁池末尾，添加锁
        {
            lock_pool.lock_list->next = newlock;
            lock_pool.lock_count++;
            return;
        }
    }
}

void leave_critical_section(PCB* process,char file_addr[]) //离开共享区，释放锁
{
    int i=0;
    int count=lock_pool.lock_count;
    LOCK *prev = NULL;
    LOCK *curr = lock_pool.lock_list;
    for(; i< count; i++) //查找该进程占据的该文件的锁
    {
        if( curr->pid == process->pid && strcmp(curr->shared_file_addr,file_addr) == 0)
        {
            if(curr == lock_pool.lock_list) //如果该锁是锁池的第一个锁
                lock_pool.lock_list = curr->next;
            else if(curr->next == NULL) //该锁是锁池最后一个锁
                prev->next = NULL;
            else //该锁在中间位置
                prev->next = curr->next;
            lock_pool.lock_count--;
            free(curr);
            return;
        }
        prev=curr;
        curr=curr->next;
    }
}

int main() 
{
    init_lock_pool();
    PCB* p1=create_process(1, 5, "test1", 0, 2);
    PCB* p2=create_process(2, 5, "test1", 0, 3);
    enter_critical_section(p1,'w',"D://test.txt");
    enter_critical_section(p2,'r',"D://test.txt");
    leave_critical_section(p1,"D://test.txt");
    enter_critical_section(p2,'r',"D://test.txt");
    enter_critical_section(p1,'r',"D://test.txt");
}