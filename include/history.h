#ifndef HISTORY_H
#define HISTORY_H

typedef struct
{
    char *entries[1000]; // MAX_HISTORY
    int count;           // 已记录总数
    int start;           // 循环数组起始索引
} History;

void init_history(void);
void add_to_history(const char *cmd);
void print_history(void);
void print_history_n(int n); // 显示最近 n 条历史
char *expand_history(const char *cmd);
void free_history(void);

#endif