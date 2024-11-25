#include <fcntl.h>

#define tgt(x) static const typeof(x) HOST_##x = x

tgt(O_ACCMODE);
tgt(O_WRONLY);
tgt(O_RDWR);
tgt(O_CREAT);
tgt(O_EXCL);
tgt(O_NOCTTY);
tgt(O_TRUNC);
tgt(O_APPEND);
tgt(O_NONBLOCK);
tgt(O_SYNC);
tgt(O_DSYNC);
tgt(O_DIRECTORY);
tgt(O_NOFOLLOW);
tgt(O_DIRECT);
tgt(O_CLOEXEC);
tgt(O_PATH);

#include "qemu/osdep.h"
#include "qemu.h"

/* Utility function: Table-driven functions to translate bitmasks
 * between host and target formats
 */
unsigned int target_to_host_bitmask_len(unsigned int target_mask,
                                        const bitmask_transtbl *tbl,
                                        size_t len)
{
    unsigned int host_mask = 0;

    for (size_t i = 0; i < len; ++i) {
        if ((target_mask & tbl[i].target_mask) == tbl[i].target_bits) {
            host_mask |= tbl[i].host_bits;
        }
    }
    return host_mask;
}

const bitmask_transtbl fcntl_flags_tbl[19] = {
  { O_ACCMODE,   O_WRONLY,    HOST_O_ACCMODE,   HOST_O_WRONLY,    },
  { O_ACCMODE,   O_RDWR,      HOST_O_ACCMODE,   HOST_O_RDWR,      },
  { O_CREAT,     O_CREAT,     HOST_O_CREAT,     HOST_O_CREAT,     },
  { O_EXCL,      O_EXCL,      HOST_O_EXCL,      HOST_O_EXCL,      },
  { O_NOCTTY,    O_NOCTTY,    HOST_O_NOCTTY,    HOST_O_NOCTTY,    },
  { O_TRUNC,     O_TRUNC,     HOST_O_TRUNC,     HOST_O_TRUNC,     },
  { O_APPEND,    O_APPEND,    HOST_O_APPEND,    HOST_O_APPEND,    },
  { O_NONBLOCK,  O_NONBLOCK,  HOST_O_NONBLOCK,  HOST_O_NONBLOCK,  },
  { O_SYNC,      O_DSYNC,     HOST_O_SYNC,      HOST_O_DSYNC,     },
  { O_SYNC,      O_SYNC,      HOST_O_SYNC,      HOST_O_SYNC,      },
  { O_DIRECTORY, O_DIRECTORY, HOST_O_DIRECTORY, HOST_O_DIRECTORY, },
  { O_NOFOLLOW,  O_NOFOLLOW,  HOST_O_NOFOLLOW,  HOST_O_NOFOLLOW,  },
  { O_DIRECT,    O_DIRECT,    HOST_O_DIRECT,    HOST_O_DIRECT,    },
  { O_CLOEXEC,   O_CLOEXEC,   HOST_O_CLOEXEC,   HOST_O_CLOEXEC    },
  { O_PATH,      O_PATH,      HOST_O_PATH,      HOST_O_PATH       },
};

