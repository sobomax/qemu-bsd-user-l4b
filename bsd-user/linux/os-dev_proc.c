#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "os-dev_proc.h"

#define MEMC_BUF_SIZE 4096

struct meminfo_entry
get_meminfo_value(const char *name)
{
    struct meminfo_entry result = {.error = -1};
    char rbuf[MEMC_BUF_SIZE];
    int name_len = strlen(name);

    int fd = open("/proc/meminfo", O_RDONLY);
    if (fd < 0) {
        return result;
    }

    ssize_t rlen = read(fd, rbuf, sizeof(rbuf));
    close(fd);
    if (rlen <= 0) {
        return result;
    }

    const char *cp = rbuf;
    while (cp < rbuf + rlen) {
        cp = memmem(cp, rlen - (cp - rbuf) - 1, name, name_len);
        if (cp == NULL) {
            // Not found
            errno = ENOENT;
            break;
        }
        if (cp != rbuf && (cp[-1] != '\n' || cp[name_len] != ':')) {
            cp += name_len;
            continue;
        }
        cp += name_len + 1;
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
