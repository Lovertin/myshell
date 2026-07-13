#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "shell.h"

// 执行整条管道命令，返回退出状态
int execute_pipeline(Pipeline *pipeline);

// 执行单条命令，返回退出状态
int execute_single(SimpleCommand *cmd, int background);

// 执行复合命令（支持 ;、&&、||），返回最终退出状态
int execute_compound(CompoundCommand *compound);

#endif