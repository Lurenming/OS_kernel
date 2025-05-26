#define _CRT_SECURE_NO_WARNINGS
/* -------- log.c -------- */
#include "log.h"
#include "filemanagement.h"  // 包含block_write/block_read声明
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int log_position = 0;  // 当前写入位置（环形缓冲区）

/* 初始化日志存储区 */
void log_init_disk() {
    // 清空所有日志块
    char empty_block[BLOCK_SIZE] = { 0 };
    for (int i = 0; i < LOG_SIZE; i++) {
        block_write(LOG_START + i, empty_block);
    }
    log_position = 0;

    // 写入初始化日志（使用自身日志系统）
    LogEntry init_entry = {
        .timestamp = time(NULL),
        .line = __LINE__,
        .level = "INFO",
        .file = "log.c",
        .message = "Initailized log system"
    };
    block_write(LOG_START, (char*)&init_entry);
}

const char* get_short_filename(const char* path) {
    const char* unix_slash = strrchr(path, '/');
    const char* win_slash = strrchr(path, '\\');
    const char* slash = NULL;

    // 比较两种分隔符的位置
    if (unix_slash && win_slash) {
        slash = (unix_slash > win_slash) ? unix_slash : win_slash;
    }
    else if (unix_slash) {
        slash = unix_slash;
    }
    else {
        slash = win_slash;
    }

    return slash ? (slash + 1) : path;
}

/* 写入日志到磁盘块 */
void log_write_disk(LogLevel level, const char* file, int line, const char* fmt, ...) {
    LogEntry entry;
    memset(&entry, 0, sizeof(entry));

    // 设置基础信息
    entry.timestamp = time(NULL);
    entry.line = line;

    // 处理日志级别
    const char* level_str = (level == LOG_INFO) ? "INFO" : "ERROR";
    strncpy(entry.level, level_str, sizeof(entry.level) - 1);

    // 获取纯文件名
    const char* short_file = get_short_filename(file);

    // 安全拷贝文件名（保证15字符+终止符）
    strncpy(entry.file, short_file, sizeof(entry.file) - 1);
    entry.file[sizeof(entry.file) - 1] = '\0'; // 强制终止

    // 格式化消息内容
    va_list args;
    va_start(args, fmt);
    vsnprintf(entry.message, sizeof(entry.message) - 1, fmt, args);
    va_end(args);

    // 计算写入位置（环形缓冲区）
    int target_block = LOG_START + (log_position % LOG_SIZE);

    // 写入磁盘
    if (block_write(target_block, (char*)&entry)) {
        log_position++;
    }
    else {
        fprintf(stderr, "Failed to write log block %d\n", target_block);
    }
}

/* 显示所有日志内容 */
void log_display_all() {
    printf("\n=== Disk Log Dump ===\n");

    // 按写入顺序读取
    for (int i = 0; i < LOG_SIZE; i++) {
        int block_num = LOG_START + ((log_position + i) % LOG_SIZE);

        LogEntry entry;
        if (block_read(block_num, (char*)&entry)) {
            // 跳过空条目
            if (entry.timestamp == 0) continue;

            // 转换时间戳
            char time_buf[20];
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S",
                localtime(&entry.timestamp));

            // 打印条目
            printf("[%s][%s][%s:%d] %s\n",
                time_buf,
                entry.level,
                entry.file,
                entry.line,
                entry.message);
        }
    }
    printf("=====================\n");
}

/* 清空日志区 */
void log_clear_disk() {
    log_init_disk();  // 重用初始化逻辑
}