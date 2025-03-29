#include "process_manage.c"

// 本.c文件是一个针对进程调度的测试程序

int main() 
{
    initqueue();
    FILE* fp = fopen("process_input.txt", "r");
    if(fp != NULL) printf("打开文件\"process_input.txt\"成功！\n");
    else {
        printf("文件打开失败！\n");
        exit(0);
    }
    int pid, priority, start_time, run_time;
    char name[20];
    while(fscanf(fp, "%d %d %s %d %d", &pid, &priority, name, &start_time, &run_time) != EOF){
        PCB* p = create_process(pid, priority, name, start_time, run_time);// pid，优先级，名称，开始时间，运行时间
        add_to_queue(p_start_queue, p);
    }

    printf("请输入你所需要的进程调度算法序号：\n");
    printf("0 -- FCFS \n1 -- SJF2\n2 -- 优先级\n3 -- RR\n");
    scanf("%d", &scheNum);
    printf("请输入算法是否是抢占式的：\n0 -- 非抢占式\n1 -- 抢占式\n");
    scanf("%d", &mode);
    run();

    /*
    add_to_queue(p_ready_queue,p1);
    add_to_queue(p_ready_queue,p2);

    execute_process();
    */
}