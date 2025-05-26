#include "filemanagement.h"
#include "log.h"

void handle_mkdir(const char* path) {
    int inode = create_entry(path, 1, 0755);
    if (inode == -1) {
        printf("Mkdir failed %s\n", path);
        log_write_disk(LOG_ERROR, __FILE__, __LINE__, "Create %s failed", path);
    }
    else {
        printf("Mkdir succeeded %s\n", path);
        log_write_disk(LOG_INFO, __FILE__, __LINE__, "Creating %s", path);
    }
}

void handle_log() {
    log_display_all();
}

void handle_log_clear() {
    log_clear_disk();
    printf("Log cleared\n");
}

void handle_touch(const char* path) {
    int inode = create_entry(path, 0, 0644);
    if (inode == -1) {
        printf("Fail to create a file: %s\n", path);
        log_write_disk(LOG_ERROR, __FILE__, __LINE__, "Create %s failed", path);
    }
    else {
        printf("Create a file successfully: %s\n", path);
        log_write_disk(LOG_INFO, __FILE__, __LINE__, "Creating %s", path);
    }
}

void handle_rm(const char* path) {
    if (delete_entry(path) == -1) {
        printf("Fail to delete: %s\n", path);
        log_write_disk(LOG_ERROR, __FILE__, __LINE__, "Delete %s failed", path);
    }
    else {
        printf("Delete successfully: %s\n", path);
        log_write_disk(LOG_INFO, __FILE__, __LINE__, "Delete %s successfully", path);
    }
}

void handle_ls(const char* path) {
    int inode_num;
    const char* target_path = path ? path : "/";
    if (resolve_path(target_path, &inode_num) != 0) {
        printf("Path not exists: %s\n", target_path);
        log_write_disk(LOG_ERROR, __FILE__, __LINE__, "ls %s failed, path not exists", path);
        return;
    }
    dir_ls(inode_num);
    log_write_disk(LOG_INFO, __FILE__, __LINE__, "ls %s", path);
}

void handle_write(const char* path, const char* data) {
    OS_FILE* f = Open_File(path, RDWR);
    if (!f) {
        printf("Can't open the file: %s\n", path);
        return;
    }
    int written = file_write(f, data, strlen(data));
    Close_File(f);
    if (written < 0) {
        printf("Write failed\n");
        log_write_disk(LOG_ERROR, __FILE__, __LINE__, "Write %s failed", path);
    }
    else {
        printf("Successfully write %d bytes to %s\n", written, path);
        log_write_disk(LOG_INFO, __FILE__, __LINE__, "Write %d bytes to %s", written, path);
    }
    log_write_disk(LOG_INFO, __FILE__, __LINE__, "Close file %s ", path);
}

void handle_read(const char* path) {
    OS_FILE* f = Open_File(path, RDONLY);
    if (!f) {
        printf("Can't open the file: %s\n", path);
        return;
    }
    char buf[MAX_FILE_SIZE + 1] = { 0 };
    int read_bytes = file_read(f, buf, MAX_FILE_SIZE);
    Close_File(f);
    if (read_bytes < 0) {
        printf("Fail to read\n");
        log_write_disk(LOG_ERROR, __FILE__, __LINE__, "Fail to Read %s ", path);
    }
    else {
        printf("File's contents (%d bytes):\n%.*s\n", read_bytes, read_bytes, buf);
        log_write_disk(LOG_INFO, __FILE__, __LINE__, "Read %s ", path);
    }
    log_write_disk(LOG_INFO, __FILE__, __LINE__, "Close file %s ", path);
}

void print_help() {
    printf("  Available commands:\n");
    //printf("  run       \运行进程\n")
    printf("  mkdir <Your path>                 Create a catalog \n");
    printf("  touch <Your path>                 Create a file\n");
    printf("  rm <Your path>                    Delete file or catalog\n");
    printf("  ls <Your path>                    List contents of a catalog\n");
    printf("  write <Your path> <Your data>     Write a file\n");
    printf("  read <Your path>                  Read a file\n");
    printf("  log                               Print the log\n");
    printf("  help                              Show help information\n");
    printf("  exit                              Quit system\n");
}
