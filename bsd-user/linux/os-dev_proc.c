#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/user.h> // For PAGE_SIZE
#include <stdio.h>    // For perror

struct meminfo_entry {
    int error; // 0 on success, non-zero on failure
    long long value;
};

struct meminfo_entry
get_meminfo_value(const char *name)
{
    struct meminfo_entry result = {.error = -1};
    char rbuf[PAGE_SIZE];
    int name_len = strlen(name);

    int fd = open("/proc/meminfo", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return result;
    }

    ssize_t rlen = read(fd, rbuf, sizeof(rbuf));
    close(fd);
    if (rlen <= 0) {
        perror("read");
        return result;
    }

    const char *cp = rbuf;
    while (cp < rbuf + rlen) {
        cp = memmem(cp, rlen - (cp - rbuf), name, name_len);
        if (cp == NULL) {
            // Not found
            errno = ENOENT;
            break;
        }
        if (cp != rbuf && cp[-1] != '\n') {
            cp += name_len;
            continue;
        }
        cp += name_len;
        const char *el = memchr(cp, '\n', rlen - (cp - rbuf));
        if (el == NULL)
            el = rbuf + rlen;
        char *tp;
        long long val = strtoll(cp, &tp, 10);
        if (tp == cp || tp > el) {
	    errno = EINVAL;
            break;
        }
        // Skip any whitespace
        for (; tp < el && isspace((unsigned char)*tp); tp++)
            ;
        if (tp == el) {
            errno = EINVAL;
            break;
        }
        if (strncmp(tp, "kB", 2) != 0) {
            errno = EINVAL;
            break;
        }
        result.value = val * 1024;
        result.error = 0;
        break;
    }
    return result;
}
