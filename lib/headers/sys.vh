
#if OS == linux

link_dynamic "pthread"
link_dynamic "c"
link_dynamic ":libc_nonshared.a"
link_dynamic ":ld-linux-x86-64.so.2"

// header "linux/structs"
header "linux/abi"
// header "linux-[ARCH]/enum"
#if ARCH == arm64
header "linux/arm64/libc-gen"
header "linux/arm64/enum"
#else
header "linux/x64/libc-gen"
header "linux/x64/enum"
#end
header "pthread"

#elif OS == macos

link_dynamic "System"

header "macos/structs"
header "macos/abi"
//header "macos-[ARCH]/enum"
#if ARCH == arm64
header "macos/arm64/enum"
header "macos/x64/libc-gen"
#else
header "macos/x64/enum"
header "macos/x64/libc-gen"
#end
header "pthread"

#elif OS == win

link_dynamic "kernel32"
link_dynamic "libucrt"
link_dynamic "WS2_32"
link_dynamic "msvcrt"
link_dynamic "libvcruntime"

header "win/structs"
header "win/abi"
//header "win-[ARCH]/enum"
header "win/x64/enum"

#end
