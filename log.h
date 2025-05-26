/* -------- log.h -------- */
#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <time.h>

// 日志级别枚举
typedef enum {
    LOG_INFO,     // 普通信息
    LOG_ERROR     // 错误信息
} LogLevel;

// 日志条目结构（严格对齐84字节块大小）
#pragma pack(push, 1)
typedef struct {
    time_t timestamp;   // 8字节 时间戳
    uint16_t line;      // 2字节 行号
    char level[6];      // 6字节 级别标识（INFO/ERROR）
    char file[16];      // 16字节 源文件名（取最后15字符）
    char message[52];   // 52字节 日志内容
} LogEntry;             // 总大小 8+2+6+16+52 = 84字节
#pragma pack(pop)

/* 磁盘日志操作接口 */
void log_init_disk();          // 初始化日志区
void log_write_disk(LogLevel level, const char* file, int line, const char* fmt, ...); // 写入日志
void log_display_all();        // 显示所有日志
void log_clear_disk();         // 清空日志区
const char* get_short_filename(const char* path);

#endif // LOG_H
