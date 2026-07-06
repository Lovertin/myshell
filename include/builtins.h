#ifndef BUILTINS_H
#define BUILTINS_H

#include "shell.h"

// 判断是否为内建命令
int is_builtin(const char *cmd_name);

// 执行内建命令，返回 0 成功，非 0 失败
int execute_builtin(SimpleCommand *cmd);

#endif
