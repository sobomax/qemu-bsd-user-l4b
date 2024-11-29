if not have_bsd_user
   subdir_done()
endif

bsd_user_ss = ss.source_set()

common_user_inc += include_directories('include')

bsd_user_ss.add(files(
  'bsd-ioctl.c',
  'bsd-mem.c',
  'bsd-proc.c',
  'bsd-socket.c',
  'bsdload.c',
  'elfload.c',
  'main.c',
  'mmap.c',
  'thunk.c',
  'signal.c',
  'uaccess.c',
))

elf = cc.find_library('elf', required: true)
if host_os != 'linux'
  bsd_user_ss.add(files('bsd-misc.c'))
  procstat = cc.find_library('procstat', required: true)
  kvm = cc.find_library('kvm', required: true)
  bsd_user_ss.add(elf, procstat, kvm)
else
  bsd_user_ss.add(elf)
endif

# Pull in the OS-specific build glue, if any
subdir(host_os)

specific_ss.add_all(when: 'CONFIG_BSD_USER', if_true: bsd_user_ss)
