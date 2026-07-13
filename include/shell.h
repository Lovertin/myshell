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
#include "job.h"

#define MAX_ARGS 64
#define MAX_PIPES 16
#define MAX_HISTORY 1000
#define MAX_ALIAS 128
#define MAX_CMD_LEN 1024
#define MAX_COMPOUND 16

/* 单条简单命令 */
typedef struct
{
    char *args[MAX_ARGS]; // 参数列表，args[0] 为命令名
    int argc;             // 参数个数
    char *input_file;     // 输入重定向文件（< file），NULL 表示无
    char *output_file;    // 输出重定向文件（> file），NULL 表示无
    int append;           // 1 表示追加模式（>>），0 表示覆盖
    char *error_file;     // 错误重定向文件（2> file），NULL 表示无
    int error_append;     // 1 表示错误追加模式（2>>），0 表示覆盖
    int stderr_to_stdout; // 1 表示 2>&1（stderr 合并到 stdout）
    char *both_file;      // 同时重定向输出和错误（&> file），NULL 表示无
    int both_append;      // 1 表示 &>>，0 表示 &>
} SimpleCommand;

/* 完整命令行（可含管道） */
typedef struct
{
    SimpleCommand commands[MAX_PIPES]; // 管道连接的多条简单命令
    int num_commands;                  // 命令段数
    int background;                    // 1 表示后台运行（&）
} Pipeline;

/* 复合命令操作符 */
typedef enum
{
    OP_NONE = 0, // 无操作符（第一个命令之前）
    OP_SEQ,      // ;  顺序执行
    OP_AND,      // && 逻辑与
    OP_OR        // || 逻辑或
} CompoundOp;

/* 复合命令（由 ;、&&、|| 连接的多个 Pipeline） */
typedef struct
{
    Pipeline pipelines[MAX_COMPOUND];
    CompoundOp operators[MAX_COMPOUND]; // operators[0] 为 OP_NONE
    int num_pipelines;
} CompoundCommand;

// 提示符
char *get_prompt(void);

// 信号处理
void setup_signals(void);
void sigchld_handler(int sig);

// 清理函数
void cleanup(void);

#endif