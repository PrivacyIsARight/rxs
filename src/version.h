/* XMRig
 * Copyright (c) 2018-2026 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2026 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RXS_VERSION_H
#define RXS_VERSION_H

#define APP_ID        "rxs"
#define APP_NAME      "rxs"
#define APP_DESC      "RandomX Miner"
#define APP_VERSION   "0.0.1"
#define APP_DOMAIN    ""
#define APP_SITE      ""
#define APP_COPYRIGHT ""
#define APP_KIND      "miner"

#define APP_VER_MAJOR 0
#define APP_VER_MINOR 0
#define APP_VER_PATCH 1

#ifdef RXS_OS_MACOS
#    define APP_OS "macOS"
#elif defined RXS_OS_ANDROID
#    define APP_OS "Android"
#elif defined RXS_OS_LINUX
#    define APP_OS "Linux"
#elif defined RXS_OS_FREEBSD
#    define APP_OS "FreeBSD"
#elif defined RXS_OS_OPENBSD
#    define APP_OS "OpenBSD"
#else
#    define APP_OS "Unknown OS"
#endif

#define STR(X) #X
#define STR2(X) STR(X)

#ifdef RXS_ARM
#   define APP_ARCH "ARMv" STR2(RXS_ARM)
#elif defined(RXS_RISCV)
#   define APP_ARCH "RISC-V"
#else
#   if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
#       define APP_ARCH "x86-64"
#   else
#       define APP_ARCH "x86"
#   endif
#endif

#ifdef RXS_64_BIT
#   define APP_BITS "64 bit"
#else
#   define APP_BITS "32 bit"
#endif

#endif // RXS_VERSION_H
