#include "filemanagement.h"

void handle_mkdir(const char* path) {
    int inode = create_entry(path, 1, 0755);
    if (inode == -1) {
        printf("创建目录失败: %s\n", path);
    }
    else {
        printf("目录创建成功: %s\n", path);
    }
}

void handle_touch(const char* path) {
    int inode = create_entry(path, 0, 0644);
    if (inode == -1) {
        printf("创建文件失败: %s\n", path);
    }
    else {
        printf("文件创建成功: %s\n", path);
    }
}

void handle_rm(const char* path) {
    if (delete_entry(path) == -1) {
        printf("删除失败: %s\n", path);
    }
    else {
        printf("成功删除: %s\n", path);
    }
}

void handle_ls(const char* path) {
    int inode_num;
    const char* target_path = path ? path : "/";
    if (resolve_path(target_path, &inode_num) != 0) {
        printf("路径不存在: %s\n", target_path);
        return;
    }
    dir_ls(inode_num);
}

void handle_write(const char* path, const char* data) {
    OS_FILE* f = Open_File(path, RDWR);
    if (!f) {
        printf("无法打开文件: %s\n", path);
        return;
    }
    int written = file_write(f, data, strlen(data));
    Close_File(f);
    if (written < 0) {
        printf("写入失败\n");
    }
    else {
        printf("成功写入 %d 字节到 %s\n", written, path);
    }
}

void handle_read(const char* path) {
    OS_FILE* f = Open_File(path, RDONLY);
    if (!f) {
        printf("无法打开文件: %s\n", path);
        return;
    }
    char buf[MAX_FILE_SIZE + 1] = { 0 };
    int read_bytes = file_read(f, buf, MAX_FILE_SIZE);
    Close_File(f);
    if (read_bytes < 0) {
        printf("读取失败\n");
    }
    else {
        printf("文件内容 (%d bytes):\n%.*s\n", read_bytes, read_bytes, buf);
    }
}

void print_help() {
    printf("\n可用命令:\n");
    printf("  mkdir <路径>       创建目录\n");
    printf("  touch <路径>       创建文件\n");
    printf("  rm <路径>          删除文件或目录\n");
    printf("  ls [路径]          列出目录内容\n");
    printf("  write <路径> <数据> 写入文件\n");
    printf("  read <路径>        读取文件\n");
    printf("  help              显示帮助信息\n");
    printf("  exit              退出系统\n\n");
}

int main() {
    // 初始化文件系统
    printf("初始化文件系统...\n");
    if (!disk_format() || !disk_init()) {
        printf("初始化失败!\n");
        return 1;
    }
    printf("文件系统初始化完成\n");

    char input[256];
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
            else printf("用法: mkdir <绝对路径>\n");
        }
        else if (strcmp(cmd, "touch") == 0) {
            char* path = strtok(NULL, " ");
            if (path) handle_touch(path);
            else printf("用法: touch <绝对路径>\n");
        }
        else if (strcmp(cmd, "rm") == 0) {
            char* path = strtok(NULL, " ");
            if (path) handle_rm(path);
            else printf("用法: rm <绝对路径>\n");
        }
        else if (strcmp(cmd, "ls") == 0) {
            char* path = strtok(NULL, " ");
            handle_ls(path);
        }
        else if (strcmp(cmd, "write") == 0) {
            char* path = strtok(NULL, " ");
            char* data = strtok(NULL, "");
            if (path && data) handle_write(path, data);
            else printf("用法: write <绝对路径> <数据内容>\n");
        }
        else if (strcmp(cmd, "read") == 0) {
            char* path = strtok(NULL, " ");
            if (path) handle_read(path);
            else printf("用法: read <绝对路径>\n");
        }
        else {
            printf("未知命令: %s (输入help查看帮助)\n", cmd);
        }
    }

    printf("\n文件系统退出\n");
    return 0;
}
