#pragma once

struct meminfo_entry {
    int error; // 0 on success, non-zero on failure
    long long value;
};

struct meminfo_entry get_meminfo_value(const char *name);
