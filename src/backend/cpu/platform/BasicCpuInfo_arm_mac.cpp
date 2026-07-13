/* XMRig
 * Copyright (c) 2018-2025 SChernykh <https://github.com/SChernykh>
 * Copyright (c) 2016-2025 XMRig <https://github.com/xmrig>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "backend/cpu/platform/BasicCpuInfo.h"

#include <sys/sysctl.h>


void rxs::BasicCpuInfo::init_arm()
{
#if defined(__ARM_FEATURE_CRYPTO)
    m_flags.set(FLAG_AES, true);
#endif

    size_t size = sizeof(m_brand);
    if (sysctlbyname("machdep.cpu.brand_string", m_brand, &size, nullptr, 0) != 0) {
        size = sizeof(m_brand);
        sysctlbyname("hw.model", m_brand, &size, nullptr, 0);
    }
}
