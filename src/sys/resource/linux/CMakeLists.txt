add_entrypoint_object(
  getrlimit
  SRCS
    getrlimit.cpp
  HDRS
    ../getrlimit.h
  DEPENDS
    libc.include.sys_resource
    libc.include.sys_syscall
    libc.src.__support.OSUtil.osutil
    libc.src.errno.errno
)

add_entrypoint_object(
  setrlimit
  SRCS
    setrlimit.cpp
  HDRS
    ../setrlimit.h
  DEPENDS
    libc.include.sys_resource
    libc.include.sys_syscall
    libc.src.__support.OSUtil.osutil
    libc.src.errno.errno
)
