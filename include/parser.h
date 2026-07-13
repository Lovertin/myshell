#ifndef PARSER_H
#define PARSER_H

#include "shell.h"

// 解析完整命令行，返回 Pipeline 结构
Pipeline parse_command(const char *cmd_line);

// 解析复合命令（支持 ;、&&、||）
CompoundCommand parse_compound(const char *cmd_line);

// 释放 Pipeline 内存
void free_pipeline(Pipeline *pipeline);

// 释放复合命令内存
void free_compound(CompoundCommand *compound);

#endif