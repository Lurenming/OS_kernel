#include "process_manage.h"

void initqueue()  //将三个队列初始化，给队列名称赋相应的值
{
    strcpy(start_queue.queue_name, "start");
    start_queue.head = start_queue.tail = NULL;
    start_queue.pcb_count = 0;
    
    strcpy(ready_queue.queue_name, "ready");
    ready_queue.head = ready_queue.tail = NULL;
    ready_queue.pcb_count = 0;
    
    strcpy(running_queue.queue_name, "running");
    running_queue.head = running_queue.tail = NULL;
    running_queue.pcb_count = 0;
    
    strcpy(blocked_queue[0].queue_name, "lock_blocked");
    blocked_queue[0].head = blocked_queue[0].tail = NULL;
    blocked_queue[0].pcb_count = 0;

    strcpy(blocked_queue[1].queue_name, "io_blocked");
    blocked_queue[1].head = blocked_queue[1].tail = NULL;
    blocked_queue[1].pcb_count = 0;

    strcpy(blocked_queue[2].queue_name, "memory_blocked");
    blocked_queue[2].head = blocked_queue[1].tail = NULL;
    blocked_queue[2].pcb_count = 0;
}

PCB* create_process(int pid, int priority, char process_name[10], int start_time_slot, int time_slot) //创建进程的函数,将进程初始状态赋值为NULL
{
    if (process_count >= MAX_PROCESSES) //如果大于CPU可接受的最大进程数，返回
    {
        printf("CPU无法创建更多进程,等待当前进程运行结束。\n");
        return NULL;
    }
    PCB *new_process = (PCB *)malloc(sizeof(PCB));
    new_process->pid = pid;
    strcpy(new_process->process_name,process_name);
    new_process->start_time_slot = CPU_TIME_SLOT * start_time_slot;
    new_process->process_time_slot = CPU_TIME_SLOT * time_slot;
    new_process->priority = priority;
    new_process->next = NULL;
    strcpy(new_process->state ,"NULL");
    process_count++;
    return new_process;
}

void add_to_queue(QUEUE *queue, PCB *process) //将PCB加入队列的函数
{
    process->next = NULL;
    if (queue->pcb_count == 0) //如果该队列是空，只需要将队列首尾指针都指向PCB
        queue->head = queue->tail = process;
    else //如果不为空，修改尾指针
    {
        queue->tail->next = process;
        queue->tail = process;
    }
    strcpy(process->state, queue->queue_name); //修改进程状态为相应的队列状态
    printf("运行时间:%d   PID为 %d ,名称为 %s 的进程已添加到 %s 队列。\n",timer,process->pid,process->process_name,queue->queue_name);
    (queue->pcb_count)++;
}

void remove_from_queue(QUEUE *queue, PCB *process)  // 将PCB移除出队列的函数,返回值为该PCB
{
    if ( strcmp(queue->queue_name,process->state) != 0) //如果队列名称与进程状态不符，说明不在此队列，返回

    return;
    PCB *prev = NULL, *curr = queue->head; //创建两个指针便于遍历队列，找到进程
    while (curr) 
    {
        if (curr->pid == process->pid) //找到了该进程
        {
            if (queue->pcb_count==1) // 如果队列中只含一个进程，将队列置空即可
                queue->head = NULL;
            else
            {
                if (curr == queue->tail) // 如果该进程在队列尾部，将尾指针指向prev，即尾指针前移
                {
                    queue->tail = prev;
                    prev ->next = NULL;
                }
                else if(curr == queue->head) // 如果该进程在队列头部，将头指针指向curr的下一个
                    queue->head = curr->next;
                else // 在中间位置，将prev指向curr的下一个
                    prev->next = curr->next; 
            }
            strcpy(process->state ,"NULL");
            printf("运行时间:%d   PID为 %d ,名称为 %s 的进程已从 %s 队列中移除。\n",timer,process->pid,process->process_name,queue->queue_name);
            queue->pcb_count--;
            return;
        }
        prev = curr;
        curr = curr->next;

    }
}

PCB* priority_dispatch() //优先级调度算法，当就绪队列非空时，返回查找到的优先级最高的进程
{
    PCB *curr = ready_queue.head;
    int tem = curr->priority;
    PCB *dispatched_process = curr;
    while(curr!=NULL) //遍历查找优先级最高，即数值最低的进程
    {
        if(curr->priority < tem) // 只有当优先级小时，才会切换；优先级相同时，不会切换
        {
            tem = curr->priority;
            dispatched_process=curr;
        }
        curr = curr->next;
    }
    return dispatched_process;
}

PCB* check_preemption(PCB* process_in_execute) // 检查是否需要抢占，并执行抢占逻辑
{
    PCB* higher_priority_process = priority_dispatch(); // 找到优先级最高的进程，不存在则返回NULL
    if (higher_priority_process && higher_priority_process->priority < process_in_execute->priority) 
    {
        // 如果ready中选出的最高优先级比现在执行的优先级还高
        printf("运行时间:%d   PID为 %d ,名称为 %s 的进程被更高优先级进程PID为 %d ,名称为 %s 的进程抢占！\n",timer,
        process_in_execute->pid, process_in_execute->process_name,
        higher_priority_process->pid, higher_priority_process->process_name);

        // 让当前进程回到 ready_queue
        remove_from_queue(p_running_queue, process_in_execute);
        add_to_queue(p_ready_queue, process_in_execute);

        // 让高优先级进程执行
        remove_from_queue(p_ready_queue, higher_priority_process);
        add_to_queue(p_running_queue, higher_priority_process);

        return higher_priority_process; // 返回被抢占的新进程
    }
    else{
        return process_in_execute;
    }
}

PCB* sjf_dispatch(){
    // 短进程优先调度，返回剩余时间最短的进程的指针
    PCB* curr = ready_queue.head;
    int tem = curr->process_time_slot; // 剩余运行时间
    PCB *dispatched_process = curr;

    while(curr!=NULL) //遍历查找剩余时间最小的进程
    {
        if(curr->process_time_slot < tem) // 只有当优先级小时，才会切换；优先级相同时，不会切换
        {
            tem = curr->process_time_slot;
            dispatched_process=curr;
        }
        curr = curr->next;
    }
    return dispatched_process;
}

PCB* check_shortest(PCB* process_in_execute) // 检查当前运行的进程是否是时间最短的，如果是则抢占
{
    PCB* shorter_process = sjf_dispatch(); // 找到时间更短的进程，不存在则返回NULL
    if (shorter_process && shorter_process->process_time_slot < process_in_execute->process_time_slot) 
    {
        // 如果ready中选出的进程剩余时间比现在运行的短
        printf("运行时间:%d   PID为 %d ,名称为 %s 的进程被剩余运行时间更短的进程PID为 %d ,名称为 %s 的进程抢占！\n",timer,
        process_in_execute->pid, process_in_execute->process_name,
        shorter_process->pid, shorter_process->process_name);

        // 让当前进程回到 ready_queue
        remove_from_queue(p_running_queue, process_in_execute);
        add_to_queue(p_ready_queue, process_in_execute);

        // 让高优先级进程执行
        remove_from_queue(p_ready_queue, shorter_process);
        add_to_queue(p_running_queue, shorter_process);

        return shorter_process; // 返回被抢占的新进程
    }
    else{
        return process_in_execute;
    }
}

// 执行进程
void execute_process() 
{
    while ( process_count > 0 )
    {
        PCB* process_in_execute = NULL;
        if (running_queue.pcb_count == 0)  // 无论是否是抢占模式，只要running_queue 为空，则调度进程
        {
            process_in_execute = priority_dispatch();
            if (process_in_execute) 
            {
                remove_from_queue(p_ready_queue, process_in_execute);
                add_to_queue(p_running_queue, process_in_execute);
            }
        } 
        else 
            process_in_execute = running_queue.head;

        if (!process_in_execute) // 若无可执行进程，则直接返回
        {
            printf("所有进程均已执行完毕,退出运行\n");
            return;
        }

        printf("运行时间:%d   PID为 %d ,名称为 %s 的进程开始运行\n", timer, process_in_execute->pid,process_in_execute->process_name);

        while (process_in_execute->process_time_slot > 0) //执行某个进程，如果是抢占调度，每经过一个CPU时间片就检查抢占
        {
            Sleep(CPU_TIME_SLOT);  // 执行 1 秒
            timer++;
            process_in_execute->process_time_slot--;
            if (mode == 1)
                process_in_execute = check_preemption(process_in_execute); //检查抢占
        }

        printf("运行时间:%d   PID为 %d ,名称为 %s 的进程运行结束。\n", timer,process_in_execute->pid,process_in_execute->process_name);  // 进程执行完毕，移除
        remove_from_queue(p_running_queue, process_in_execute);
        free(process_in_execute);
        process_count--;
    }
}

PCB* get_now_process(int sche, int isP){ // 这个函数用于考虑调度算法和抢占（非抢占）
    /*
    规定：sche = 0 -- FCFS
        sche = 1 -- SJF
        sche = 2 -- 优先级
        sche = 3 -- RR
        isP = 0 -- 非抢占
        isP = 1 -- 抢占
    */
    PCB* now = p_running_queue->head; // 有可能现在还没有进程在里面
    PCB* process_in_execute = NULL;
    if(now != NULL && isP == 0){
        // 正在运行一个进程，而且非抢占
        return now;
    }
    else{
        // 现在没有进程运行，或者现在有进程运行，但是抢占式
        if(p_ready_queue->pcb_count == 0){
            // 此时没有进程可供选择，需要检查运行队列，如果运行队列为空，则真空了
            if(p_running_queue->pcb_count == 0) return NULL;
            else return now; // 继续运行现在的进程
        }
        else{
            // 有可以选择的，使用调度算法
            switch(sche){
                case 0:
                    //FCFS调度算法，比较进入ready队列的时间
                    return p_ready_queue->head;
                    break;

                case 1:
                    //SJF调度算法
                    if(now == NULL) return sjf_dispatch();
                    else return check_shortest(now);
                    break;

                case 2:
                    //优先级调度算法，在ready队列中，找优先级最高的进程
                    if(now == NULL) return priority_dispatch();
                    else return check_preemption(now);
                    break;

                case 3:
                    //轮询调度算法
                    //return RR_dispatch();
                    break;
            }
        }
    }
}

void add_to_lock_list(LOCK* newlock, LOCK_POOL *tempool) //将某个锁添加到tempool中(lock_in_use或lock_buffer)
{
    if(tempool->lock_count == 0) //锁池为空,将首个锁赋对应的值
        tempool->lock_list = newlock;
    else //否则遍历到锁池末尾，添加锁
    {
        LOCK* lock_list = tempool->lock_list;
        while(lock_list->next != NULL)
            lock_list = lock_list->next;
        lock_list->next = newlock;
    }
    tempool->lock_count++;
    if(tempool == p_lock_buffer) //如果是lock_buffer，则打印信息
        printf("运行时间:%d   PID为 %d 的进程请求锁，锁模式为 %c,文件地址为 %s\n",timer,newlock->pid,newlock->lockmode,newlock->file_addr);
    else //如果是lock_in_use
        printf("运行时间:%d   PID为 %d 的进程获得锁，锁模式为 %c,文件地址为 %s\n",timer,newlock->pid,newlock->lockmode,newlock->file_addr);
}

LOCK* remove_from_lock_list(PCB* process,char lockmode,char file_addr[],LOCK_POOL *tempool) //将某个锁从lock_buffer或lock_in_use中移除
{
    LOCK *prev=NULL;
    LOCK *temlock=tempool->lock_list;
    int count=tempool->lock_count;
    for(int i=0 ; i< count;i++)
    {
        if(temlock->pid==process->pid && temlock->lockmode==lockmode && strcmp(temlock->file_addr,file_addr)==0)
        {
            if(i==0) //是锁池中第一个元素
                tempool->lock_list = tempool->lock_list->next;
            else if(i==count-1) //是最后一个元素
                prev->next=NULL;
            else //是中间元素
                prev->next=temlock->next;
            temlock->next=NULL;
            tempool->lock_count--;
            return temlock; //返回被移除的锁
        }
        prev=temlock;
        temlock=temlock->next;
    }
    return NULL; //没有找到该锁，返回NULL
}

void enter_critical_section(PCB* process,char lockmode,char file_addr[])  //lockmode 代表进程的对共享文件的操作模式，分为w(写)和r(读)
{
    LOCK *newlock = (LOCK *)malloc(sizeof(LOCK)); //根据传入参数创建一个新的锁
    newlock->next = NULL;
    newlock->lockmode = lockmode;
    newlock->pid = process->pid;
    strcpy(newlock->file_addr,file_addr);
    add_to_lock_list(newlock,p_lock_buffer); //将新创建的锁添加到lock_buffer中
    int i=0;
    int count=lock_in_use.lock_count;
    LOCK* lock_list = lock_in_use.lock_list;
    for(; i< count; i++) //遍历lock_in_use，查找是否有进程占有该文件的锁
    {
        if(strcmp(lock_list->file_addr,file_addr) == 0) //该文件已经有进程占有锁
        {
            if(lock_list->pid == process->pid) 
            //在lock_in_use中占有该锁的是该进程，采用的策略是。否则
            {   
                if(lock_list->lockmode!=lockmode) //如果锁模式不同，使进程离开共享区，申请新锁
                    leave_critical_section(process,lock_list->lockmode,file_addr); //将该进程从共享区中移除
                else
                    return; //如果锁模式不变，则直接返回
            }
            else //占有该锁的不是该进程,而是其他进程。则根据lockmode的不同，进行不同的操作
            {
                if(lock_list->lockmode == 'w') // 其他进程以写模式占有锁
                {   
                    remove_from_queue(p_running_queue, process); //
                    add_to_queue(p_blocked_queue0, process); //将该进程从运行队列中移除,添加到阻塞队列中
                    return;
                }
                else // 其他进程进程以读模式占有锁
                {
                    if (lockmode== 'r') //如果该进程也是要求读，那么可以获得锁
                    {
                        remove_from_lock_list(process,lock_list->lockmode,file_addr,p_lock_buffer);
                        add_to_lock_list(newlock,p_lock_in_use); //将锁从lock_buffer中移除，添加到lock_in_use中
                    }
                    else // 该进程要求写，那么不允许共享锁，阻塞该进程
                    {
                        remove_from_queue(p_running_queue, process); //将该进程从就绪队列中移除
                        add_to_queue(p_blocked_queue0, process); //将该进程添加到阻塞队列中
                        return;
                    }
                }   
            }
            break;
        }
        lock_list = lock_list->next;
    }
    if(i == count) //遍历结束，没有任何进程占有锁,则该进程直接获得锁
    {
        remove_from_lock_list(process,lockmode,file_addr,p_lock_buffer);
        add_to_lock_list(newlock,p_lock_in_use); //将锁从lock_buffer中移除，添加到lock_in_use
    }
}

void check_blocked_process_lock() //检查阻塞队列中的进程，查看是否有进程可以解除阻塞
{
    PCB* curr = blocked_queue[0].head;
    int i;
    while(curr != NULL) //遍历阻塞队列
    {
        LOCK* lock_list = lock_in_use.lock_list;
        int count=lock_in_use.lock_count;
        //该过程存在两个遍历，一个是遍历阻塞队列，一个是遍历锁池，
        //先遍历锁池，再使阻塞队列指针后移，查看下一个进程是否可以解除阻塞
        //解除阻塞只有一种情况：如果锁池中没有该文件的锁，则可以解除阻塞
        for(i =0 ; i< count; i++) 
        {
            if(strcmp(lock_list->file_addr,curr->process_name) == 0) //该文件还有进程占有锁，则不能解除阻塞
                break;
            lock_list = lock_list->next;
        }
        if(i == count) //遍历结束，没有任何进程占有锁,则可以解除阻塞
        {
            remove_from_queue(p_blocked_queue0, curr); //将该进程从阻塞队列中移除
            add_to_queue(p_ready_queue, curr); //将该进程添加到就绪队列中
        }
        curr = curr->next;
    }
}

void leave_critical_section(PCB* process,char lockmode, char file_addr[]) //离开共享区，释放锁
{
    LOCK* temlock = remove_from_lock_list(process,lockmode,file_addr,p_lock_in_use); //将锁从锁池中释放
    if(temlock == NULL) //如果没有找到该锁，说明该进程没有占有该锁，直接返回
        return;
    free(temlock); //释放锁
    printf("运行时间:%d   PID为 %d ,名称为 %s 的进程释放锁，锁模式为 %c,文件地址为 %s\n",timer,process->pid,process->process_name,lockmode,file_addr);
    check_blocked_process_lock(); //检查阻塞队列中的进程，查看是否有进程可以解除阻塞
}

void run(){ // 这个函数以时间片为单位，每次经过一个时间片，就对创建的所有进程进行处理
    PCB* process_in_execute = NULL;
    PCB* process_last = NULL;
    timer = 0;

    while(1){
        // 先默认死循环
        PCB *curr =start_queue.head;

        //printf("运行时间:%d\n", timer);
        while(curr != NULL){ // 遍历start队列，将可以放入ready队列的进程放入
            if(timer >= curr ->start_time_slot){
                remove_from_queue(p_start_queue, curr);
                add_to_queue(p_ready_queue, curr);
            }
            curr = curr->next;
        }

        // 调度算法：选择一个进程来运行
        /*详细理解：调度算法包含FCFS、SJF、优先级、RR，还包含抢占式和非抢占式
        但是这些调度算法都可以化作在每一个时间片结尾进行一次选择
        详细的来说：非抢占相当于在这个程序结束前，都选择自己；
        抢占式中，每一个时间片时，都选择一个优先级相对较高的，如果自己和别人优先级相同，则运行自己*/

        PCB* process_last = p_running_queue->head; // 上一次运行的进程
        process_in_execute = get_now_process(scheNum, mode);  // 当前正在运行的进程
        // print_process(process_in_execute);
        if(process_in_execute != NULL && process_last == NULL){ // 上次运行的进程结束了
            remove_from_queue(p_ready_queue, process_in_execute);
            add_to_queue(p_running_queue, process_in_execute);
            printf("运行时间:%d   PID为 %d ,名称为 %s 的进程开始运行\n", timer, process_in_execute->pid,process_in_execute->process_name);
        }

        Sleep(CPU_TIME_SLOT);  // 执行 1 CPU时间
        timer += CPU_TIME_SLOT;

        if(process_in_execute != NULL){
            process_in_execute->process_time_slot -= CPU_TIME_SLOT;

            if(process_in_execute->process_time_slot == 0){
                // 说明这个进程运行结束了
                printf("运行时间:%d   PID为 %d ,名称为 %s 的进程运行结束。\n", timer,process_in_execute->pid,
                    process_in_execute->process_name);  // 进程执行完毕，移除
                remove_from_queue(p_running_queue, process_in_execute);
                free(process_in_execute);
                process_count--;
            }
        }
        
    }
}

void print_process(PCB* process){
    // 打印传入的进程
    printf("PID为 %d, 名称为 %s, 状态是 %s, 开始时间为 %d, 运行时间为 %d, 优先级为 %d 的进程\n",
        process->pid, process->process_name, process->state, process->start_time_slot,
        process->process_time_slot, process->priority);
}

void print_queue(QUEUE* queue){
    // 打印传入的队列
    PCB* curr = queue->head;
    printf("队列 %s 中的进程有：\n", queue->queue_name);
    if(curr == NULL) printf("队列为空！");
    while(curr != NULL){
        print_process(curr);
        curr = curr->next;
    }
}
