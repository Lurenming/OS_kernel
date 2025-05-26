#include "filemanagement.h"
//#include "process_manage.h"
//#include "memory1.h"
#include "log.h"

int file_system();
int process_system();
int memory_system();
int os_test();

int main() {
    printf("Operation System\n");
    printf("Commands:\n");
    printf("1 - Process system\n");
    printf("2 - Memory system\n");
    printf("3 - File system\n");
    printf("4 - 最后的整体测试程序\n");
    printf("e - Exit\n");
    
    char inputs[10];
    if (scanf("%s", inputs) != 1) {
        return 0;
    }

    if (strcmp(inputs, "e") == 0) {
        return 0; 
    }
    else if(strcmp(inputs, "1") == 0){
        process_system();
    }
    else if(strcmp(inputs, "2") == 0){
        memory_system();
    }
    else if(strcmp(inputs, "3") == 0){
        int test = file_system();
    }
    else if(strcmp(inputs, "4") == 0){
        ;
    }
    else{
        printf("Invalid commands,system exits");
        return 0;
    }
    return 0;
}

int file_system(){
    // 初始化文件系统
    printf("Initializing file system...\n");
    if (!disk_format() || !disk_init()) {
        printf("Initialize failed\n");
        return 1;
    }
    printf("Initialized successfully\n\n");

    char input[320];
    print_help();

    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        // 去除换行符
        input[strcspn(input, "\n")] = '\0';

        // 分割命令和参数
        char* cmd = strtok(input, " ");
        if (!cmd) continue;

        if (strcmp(cmd, "exit") == 0) {
            break;
        }
        else if (strcmp(cmd, "help") == 0) {
            print_help();
        }
        else if (strcmp(cmd, "mkdir") == 0) {
            char* path = strtok(NULL, " ");
            if (path) handle_mkdir(path);
            else printf("Usage: mkdir <Absolute Path>\n");
        }
        else if (strcmp(cmd, "touch") == 0) {
            char* path = strtok(NULL, " ");
            if (path) handle_touch(path);
            else printf("Usage: touch <Absolute Path>\n");
        }
        else if (strcmp(cmd, "rm") == 0) {
            char* path = strtok(NULL, " ");
            if (path) handle_rm(path);
            else printf("Usage: rm <Absolute Path>\n");
        }
        else if (strcmp(cmd, "ls") == 0) {
            char* path = strtok(NULL, " ");
            handle_ls(path);
        }
        else if (strcmp(cmd, "write") == 0) {
            char* path = strtok(NULL, " ");
            char* data = strtok(NULL, "");
            if (path && data) handle_write(path, data);
            else printf("Usage: write <Absolute Path> <Your Content>\n");
        }
        else if (strcmp(cmd, "read") == 0) {
            char* path = strtok(NULL, " ");
            if (path) handle_read(path);
            else printf("Usage: read <Absolute Path>\n");
        }
        else if (strcmp(cmd, "log") == 0) {
            log_display_all();
        }
        else {
            printf("Invalid Command: %s (Enter help to search valid commands)\n", cmd);
        }
    }

    printf("\nExit file system\n");
    return 0;
}

int process_system(){
    ;
}
int memory_system(){
    ;
}