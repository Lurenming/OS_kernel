#include "process_manage.c"

// 本.c文件是一个针对进程调度的测试程序

int main() 
{
    initqueue();
    PCB* p1=create_process(1, 5, "test1", 1, 2); // pid，优先级，名称，开始时间，运行时间
    PCB* p2=create_process(2, 4, "test2", 0, 3);
    PCB* p3=create_process(3, 4, "test3", 1, 5);
    PCB* p4=create_process(4, 3, "test4", 4, 3);

    add_to_queue(p_start_queue,p1);
    add_to_queue(p_start_queue,p2);
    add_to_queue(p_start_queue,p3);
    add_to_queue(p_start_queue,p4);
    //print_queue(p_start_queue);
    run();

    /*
    add_to_queue(p_ready_queue,p1);
    add_to_queue(p_ready_queue,p2);

    execute_process();
    */
}