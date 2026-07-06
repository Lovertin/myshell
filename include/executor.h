#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "shell.h"

// 执行整条管道命令
void execute_pipeline(Pipeline *pipeline);

// 执行单条命令
void execute_single(SimpleCommand *cmd, int background);

#endif
