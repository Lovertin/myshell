#ifndef ALIAS_H
#define ALIAS_H

typedef struct
{
    char *name;  // 别名
    char *value; // 对应的实际命令字符串
} AliasEntry;

typedef struct
{
    AliasEntry entries[128]; // MAX_ALIAS
    int count;
} AliasList;

void init_alias(void);
void set_alias(const char *name, const char *value);
void remove_alias(const char *name);
char *expand_alias(const char *cmd);
void print_aliases(void);
void free_alias(void);

#endif
