#include "shell.h"
#include "history.h"
#include <readline/history.h>

static History hist;

void init_history(void)
{
    memset(&hist, 0, sizeof(History));
}

void add_to_history(const char *cmd)
{
    if (hist.count < MAX_HISTORY)
    {
        hist.entries[hist.count] = strdup(cmd);
        hist.count++;
    }
    else
    {
        // 循环覆盖
        free(hist.entries[hist.start]);
        hist.entries[hist.start] = strdup(cmd);
        hist.start = (hist.start + 1) % MAX_HISTORY;
    }

    // 同时添加到 readline 历史
    add_history(cmd);
}

void print_history(void)
{
    int total = (hist.count < MAX_HISTORY) ? hist.count : MAX_HISTORY;
    int start_idx = (hist.count < MAX_HISTORY) ? 0 : hist.start;

    for (int i = 0; i < total; i++)
    {
        int idx = (start_idx + i) % MAX_HISTORY;
        printf("%5d  %s\n", i + 1, hist.entries[idx]);
    }
}

char *expand_history(const char *cmd)
{
    if (cmd[0] != '!')
    {
        return strdup(cmd);
    }

    // 解析 !n
    int n = atoi(cmd + 1);

    if (n <= 0 || n > hist.count)
    {
        fprintf(stderr, "bash: !%d: event not found\n", n);
        return NULL;
    }

    int total = (hist.count < MAX_HISTORY) ? hist.count : MAX_HISTORY;
    int start_idx = (hist.count < MAX_HISTORY) ? 0 : hist.start;

    if (n > total)
    {
        fprintf(stderr, "bash: !%d: event not found\n", n);
        return NULL;
    }

    int idx = (start_idx + n - 1) % MAX_HISTORY;
    return strdup(hist.entries[idx]);
}

void free_history(void)
{
    int total = (hist.count < MAX_HISTORY) ? hist.count : MAX_HISTORY;
    int start_idx = (hist.count < MAX_HISTORY) ? 0 : hist.start;

    for (int i = 0; i < total; i++)
    {
        int idx = (start_idx + i) % MAX_HISTORY;
        if (hist.entries[idx])
        {
            free(hist.entries[idx]);
        }
    }
}
