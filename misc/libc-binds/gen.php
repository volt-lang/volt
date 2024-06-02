<?php

include __DIR__ . '/gen-structs.php';

$imports = ['sys/stat'];
$unix_imports = ['setjmp', 'dirent', 'poll', 'sys/types', 'sys/socket', 'netdb', 'sys/time'];
$linux_imports = array_merge($imports, $unix_imports, ['sys/epoll']);
$macos_imports = array_merge($imports, $unix_imports, []);
$win_imports = array_merge($imports, ['winsock2', 'time']);

$vars = [
    'timespec' => [
        'ctype' => 'struct timespec',
        'fields' => ['tv_sec', 'tv_nsec']
    ],
    'timeval' => [
        'ctype' => 'struct timeval',
        'fields' => ['tv_sec', 'tv_usec']
    ],
    'sockaddr' => [
        'ctype' => 'struct sockaddr',
        'fields' => ['sa_family', 'sa_data']
    ],
    'pollfd' => [
        'ctype' => 'struct pollfd',
        'fields' => ['fd', 'events', 'revents']
    ],
];
$linux_vars = array_merge($vars, [
    'jmp_buf' => [
        'ctype' => 'jmp_buf',
    ],
    'stat' => [
        'ctype' => 'struct stat',
        'fields' => ['st_dev', 'st_ino', 'st_nlink', 'st_mode', 'st_uid', 'st_gid', '__pad0', 'st_rdev', 'st_size', 'st_blksize', 'st_blocks', 'st_atime', 'st_mtime', 'st_ctime', '__unused']
    ],
    'dirent' => [
        'ctype' => 'struct dirent',
        'fields' => ['d_ino', 'd_off', 'd_reclen', 'd_type', 'd_name']
    ],
    'timezone' => [
        'ctype' => 'struct timezone',
        'fields' => ['tz_minuteswest', 'tz_dsttime']
    ],
    'addrinfo' => [
        'ctype' => 'struct addrinfo',
        'fields' => ['ai_flags', 'ai_family', 'ai_socktype', 'ai_protocol', 'ai_addrlen', 'ai_addr', 'ai_canonname', 'ai_next']
    ],
    'epoll_event' => [
        'ctype' => 'struct epoll_event',
        'fields' => ['events', 'data']
    ],
]);
$macos_vars = array_merge($vars, [
    'jmp_buf' => [
        'ctype' => 'jmp_buf',
    ],
    'stat' => [
        'ctype' => 'struct stat',
        'fields' => ['st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_uid', 'st_gid', 'st_rdev', 'st_atime', 'st_mtime', 'st_ctime', 'st_size', 'st_blocks', 'st_blksize', 'st_flags', 'st_gen', 'st_lspare', '__unused']
    ],
    'dirent' => [
        'ctype' => 'struct dirent',
        'fields' => ['d_ino', 'd_off', 'd_reclen', 'd_type', 'd_name']
    ],
    'timezone' => [
        'ctype' => 'struct timezone',
        'fields' => ['tz_minuteswest', 'tz_dsttime']
    ],
    'addrinfo' => [
        'ctype' => 'struct addrinfo',
        'fields' => ['ai_flags', 'ai_family', 'ai_socktype', 'ai_protocol', 'ai_addrlen', 'ai_canonname', 'ai_addr', 'ai_next']
    ],
    'sockaddr' => [
        'ctype' => 'struct sockaddr',
        'fields' => ['sa_len', 'sa_family', 'sa_data']
    ],
]);
$win_vars = array_merge($vars, [
    'stat' => [
        'ctype' => 'struct stat',
        'fields' => ['st_gid', 'st_atime', 'st_ctime', 'st_dev', 'st_ino', 'st_mode', 'st_mtime', 'st_nlink', 'st_rdev', 'st_size', 'st_uid']
    ],
    'pollfd' => [
        'ctype' => 'WSAPOLLFD',
        'fields' => ['fd', 'events', 'revents']
    ],
    'addrinfo' => [
        'ctype' => 'ADDRINFOA',
        'fields' => ['ai_flags', 'ai_family', 'ai_socktype', 'ai_protocol', 'ai_addrlen', 'ai_addr', 'ai_canonname', 'ai_next']
    ],
]);

$targets = [
    'linux-x64' => ['target' => 'x86_64-unknown-linux-gnu', 'header_dir' => 'linux/x64', 'toolchain' => 'linux-x64/x86_64-buildroot-linux-gnu/sysroot', 'imports' => $linux_imports, 'vars' => $linux_vars],
    'macos-x64' => ['target' => 'x86_64-apple-darwin-macho', 'header_dir' => 'macos/x64', 'toolchain' => 'macos-11-3', 'imports' => $macos_imports, 'vars' => $macos_vars],
    'win-x64' => [
        'target' => 'x86_64-pc-windows-msvc',
        'header_dir' => 'win/x64',
        'toolchain' => 'win-sdk-x64',
        'clang_args' => "-ftime-trace",
        'tc_includes' => [
            'Include/10.0.22621.0/ucrt',
            'Include/10.0.22621.0/um',
            'Include/10.0.22621.0/shared',
            'MSVC/14.36.32532/include',
        ],
        'imports' => $win_imports, 'vars' => $win_vars
    ],
];

// Vars
$root = __DIR__;
$tmp = $root . '/tmp';
if (!file_exists($tmp))
    mkdir($tmp);
$dist_dir = $root . '/../../dist';
$tc_dir = $dist_dir . '/toolchains';
$header_dir = $root . '/../../lib/headers';

// Generate
foreach($targets as $valk_target => $target) {

    $code = "";
    $code .= '#pragma clang diagnostic ignored "-Wpragma-pack"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wignored-attributes"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wignored-pragma-intrinsic"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wmicrosoft-anon-tag"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wdeprecated-declarations"' . "\n";

    foreach($target['imports'] as $import) {
        $code .= "#include <$import.h>\n";
    }
    $code .= "\n";

    $i = 0;
    foreach($target['vars'] as $name => $var) {
        $code .= $var['ctype'] . " var_$name;\n";
        $i++;
    }
    $code .= "\n";

    $code .= "int main() { return 0; }\n\n";

    // Write code
    $path = $tmp . '/' . $valk_target . '.c';
    file_put_contents($path, $code);

    $path_ir = $tmp . '/' . $valk_target . '.ir';
    $ttc_dir = $tc_dir . '/' . $target['toolchain'];
    $cmd = "clang-15 -S -emit-llvm " . ($target['clang_args'] ?? '') . " $path -o $path_ir --target=" . $target['target'] . " --sysroot=$ttc_dir";
    foreach($target['tc_includes'] ?? [] as $inc) {
        $cmd .= ' -I' . $ttc_dir . '/' . $inc;
    }
    // echo $cmd . "\n";
    exec($cmd);

    var_dump($path_ir);
    $ir = file_get_contents($path_ir);

    // Gen valk structs
    $code = gen_valk_structs($ir, $target['vars']);

    $hpath = $header_dir . '/' . $target['header_dir'] . '/libc-gen.vh';
    file_put_contents($hpath, $code);
}
