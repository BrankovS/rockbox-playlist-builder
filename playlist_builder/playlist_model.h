/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef PLAYLIST_MODEL_H
#define PLAYLIST_MODEL_H

#define PB_QUEUE_MAX 10000

static int pb_move(int *items, int count, int from, int delta)
{
    int to = from + delta, tmp;
    if (from < 0 || from >= count || to < 0 || to >= count)
        return from;
    tmp = items[from]; items[from] = items[to]; items[to] = tmp;
    return to;
}

static int pb_valid_name(const char *name)
{
    const unsigned char *p = (const unsigned char *)name;
    int length = 0;
    if (!*p || *p == ' ' || *p == '.') return 0;
    for (; *p; ++p, ++length)
        if (*p < 32 || *p == 127 || *p == '/' || *p == '\\' ||
            *p == ':' || *p == '*' || *p == '?' || *p == '"' ||
            *p == '<' || *p == '>' || *p == '|') return 0;
    return length <= 120 && p[-1] != ' ' && p[-1] != '.';
}
#endif
