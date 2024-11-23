if not have_bsd_user or host_os == 'linux'
   subdir_done()
endif

runit = find_program('runit')

bsd_user_archs = [ 'aarch64', 'armv7', 'amd64', 'i386', 'powerpc', 'powerpc64', 'riscv64' ]
c = []
foreach i : bsd_user_archs
    h = 'h.' + i
    c += custom_target('bsd-user h.' + i,
        output : h,
	input : h + '.S',
	command : ['clang',
	    '-target',
	    i + '-unknown-freebsd14.0',
	    '-o',
	    '@OUTPUT@',
	    '@INPUT@',
	    '-nostdlib',
	    '-Wl,-e',
	    '-Wl,qemu_start',
	    '-static'],
	install : false)
endforeach

test('bsd-user-smoke', runit,
    args: [meson.project_build_root(), meson.current_build_dir()],
    suite: 'smoke',
    depends: c
)
