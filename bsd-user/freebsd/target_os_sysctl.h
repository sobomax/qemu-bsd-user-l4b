#pragma once

/* sysctl constants */
#if defined(__linux__)
#define        CTL_MAXNAME     24      /* largest number of components supported */

#define        CTLTYPE         0xf     /* mask for the type */
#define        CTLTYPE_NODE    1       /* name is a node */
#define        CTLTYPE_INT     2       /* name describes an integer */
#define        CTLTYPE_STRING  3       /* name describes a string */
#define        CTLTYPE_S64     4       /* name describes a signed 64-bit number */
#define        CTLTYPE_OPAQUE  5       /* name describes a structure */
#define        CTLTYPE_STRUCT  CTLTYPE_OPAQUE  /* name describes a structure */
#define        CTLTYPE_UINT    6       /* name describes an unsigned integer */
#define        CTLTYPE_LONG    7       /* name describes a long */
#define        CTLTYPE_ULONG   8       /* name describes an unsigned long */
#define        CTLTYPE_U64     9       /* name describes an unsigned 64-bit number */
#define        CTLTYPE_U8      0xa     /* name describes an unsigned 8-bit number */
#define        CTLTYPE_U16     0xb     /* name describes an unsigned 16-bit number */
#define        CTLTYPE_S8      0xc     /* name describes a signed 8-bit number */
#define        CTLTYPE_S16     0xd     /* name describes a signed 16-bit number */
#define        CTLTYPE_S32     0xe     /* name describes a signed 32-bit number */
#define        CTLTYPE_U32     0xf     /* name describes an unsigned 32-bit number */

#define        CTLFLAG_RD      0x80000000      /* Allow reads of variable */
#define        CTLFLAG_WR      0x40000000      /* Allow writes to the variable */
#define        CTLFLAG_RW      (CTLFLAG_RD|CTLFLAG_WR)
#define        CTLFLAG_DORMANT 0x20000000      /* This sysctl is not active yet */
#define        CTLFLAG_ANYBODY 0x10000000      /* All users can set this var */
#define        CTLFLAG_SECURE  0x08000000      /* Permit set only if securelevel<=0 */
#define        CTLFLAG_PRISON  0x04000000      /* Prisoned roots can fiddle */
#define        CTLFLAG_DYN     0x02000000      /* Dynamic oid - can be freed */
#define        CTLFLAG_SKIP    0x01000000      /* Skip this sysctl when listing */
#define        CTLMASK_SECURE  0x00F00000      /* Secure level */
#define        CTLFLAG_TUN     0x00080000      /* Default value is loaded from getenv() */
#define        CTLFLAG_RDTUN   (CTLFLAG_RD|CTLFLAG_TUN)
#define        CTLFLAG_RWTUN   (CTLFLAG_RW|CTLFLAG_TUN)
#define        CTLFLAG_MPSAFE  0x00040000      /* Handler is MP safe */
#define        CTLFLAG_VNET    0x00020000      /* Prisons with vnet can fiddle */
#define        CTLFLAG_DYING   0x00010000      /* Oid is being removed */
#define        CTLFLAG_CAPRD   0x00008000      /* Can be read in capability mode */
#define        CTLFLAG_CAPWR   0x00004000      /* Can be written in capability mode */
#define        CTLFLAG_STATS   0x00002000      /* Statistics, not a tuneable */
#define        CTLFLAG_NOFETCH 0x00001000      /* Don't fetch tunable from getenv() */
#define        CTLFLAG_CAPRW   (CTLFLAG_CAPRD|CTLFLAG_CAPWR)

#define CTL_SYSCTL_NAME 1

/* ... and others */
#define VM_OVERCOMMIT 12

#endif
