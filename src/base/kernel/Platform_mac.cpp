/* XMRig
 * Copyright (c) 2018-2021 SChernykh <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 XMRig <https://github.com/xmrig>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "base/kernel/Platform.h"
#include "version.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <cstdio>
#include <limits>
#include <pthread.h>
#include <sys/resource.h>
#include <uv.h>


char *rxs::Platform::createUserAgent()
{
    constexpr size_t max = 256;
    char *buf = new char[max]();

    int length = snprintf(buf, max, "%s/%s (Macintosh; macOS; %s) libuv/%s",
                          APP_NAME, APP_VERSION,
#if defined(RXS_ARM)
                          "arm64",
#else
                          "x86_64",
#endif
                          uv_version_string());

#ifdef __clang__
    snprintf(buf + length, max - static_cast<size_t>(length), " clang/%d.%d.%d",
             __clang_major__, __clang_minor__, __clang_patchlevel__);
#endif

    return buf;
}


#ifndef RXS_FEATURE_HWLOC
bool rxs::Platform::setThreadAffinity(uint64_t)
{
    // Darwin exposes affinity tags rather than hard CPU pinning. Returning
    // false accurately reports that a requested logical-CPU binding was not
    // guaranteed; hwloc supplies the best available binding when enabled.
    return false;
}
#endif


void rxs::Platform::setProcessPriority(int)
{
}


void rxs::Platform::setThreadPriority(int priority)
{
    if (priority < 0) {
        return;
    }

    constexpr int priorities[] = { 19, 5, 0, -5, -10, -15 };
    const auto index = static_cast<size_t>(priority > 5 ? 0 : priority);

    // nice(2) cannot raise an unprivileged process above its inherited
    // priority on macOS. QoS is the supported per-thread mechanism and keeps
    // sustained hashing work on the fastest available cores.
    qos_class_t qos = QOS_CLASS_DEFAULT;
    switch (priority) {
    case 0:
        qos = QOS_CLASS_BACKGROUND;
        break;
    case 1:
        qos = QOS_CLASS_UTILITY;
        break;
    case 2:
        qos = QOS_CLASS_DEFAULT;
        break;
    case 3:
    case 4:
        qos = QOS_CLASS_USER_INITIATED;
        break;
    case 5:
        qos = QOS_CLASS_USER_INTERACTIVE;
        break;
    default:
        break;
    }

    pthread_set_qos_class_self_np(qos, 0);
    setpriority(PRIO_PROCESS, 0, priorities[index]);
}


bool rxs::Platform::isOnBatteryPower()
{
    return IOPSGetTimeRemainingEstimate() != kIOPSTimeRemainingUnlimited;
}


uint64_t rxs::Platform::idleTime()
{
    const io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("IOHIDSystem"));
    if (service == IO_OBJECT_NULL) {
        return std::numeric_limits<uint64_t>::max();
    }

    const CFTypeRef property = IORegistryEntryCreateCFProperty(service, CFSTR("HIDIdleTime"), kCFAllocatorDefault, 0);
    IOObjectRelease(service);

    uint64_t idle = 0;
    if (property == nullptr || CFGetTypeID(property) != CFNumberGetTypeID() ||
        !CFNumberGetValue(static_cast<CFNumberRef>(property), kCFNumberSInt64Type, &idle)) {
        if (property != nullptr) {
            CFRelease(property);
        }
        return std::numeric_limits<uint64_t>::max();
    }

    CFRelease(property);
    return idle / 1000000U;
}
