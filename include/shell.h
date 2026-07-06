#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_ARGS 64
#define MAX_PIPES 16
#define MAX_HISTORY 1000
#define MAX_ALIAS 128
#define MAX_CMD_LEN 1024

/* 单条简单命令 */
typedef struct
{
    char *args[MAX_ARGS]; // 参数列表，args[0] 为命令名
    int argc;             // 参数个数
    char *input_file;     // 输入重定向文件（< file），NULL 表示无
    char *output_file;    // 输出重定向文件（> file），NULL 表示无
    int append;           // 1 表示追加模式（>>），0 表示覆盖
} SimpleCommand;

/* 完整命令行（可含管道） */
typedef struct
{
    SimpleCommand commands[MAX_PIPES]; // 管道连接的多条简单命令
    int num_commands;                  // 命令段数
    int background;                    // 1 表示后台运行（&）
} Pipeline;

// 提示符
char *get_prompt(void);

// 信号处理
void setup_signals(void);
void sigchld_handler(int sig);

// 清理函数
void cleanup(void);

#endif
