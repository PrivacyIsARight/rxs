/* XMRig
 * Copyright (c) 2019      Spudz76     <https://github.com/Spudz76>
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "base/io/log/backends/ConsoleLog.h"
#include "base/io/log/Log.h"
#include "base/kernel/config/Title.h"
#include "base/tools/Handle.h"

#include <cstdio>

rxs::ConsoleLog::ConsoleLog(const Title &)
{
    if (!isSupported()) {
        Log::setColors(false);
        return;
    }

    m_tty = new uv_tty_t;

    if (uv_tty_init(uv_default_loop(), m_tty, 1, 0) < 0) {
        Log::setColors(false);
        return;
    }

    uv_tty_set_mode(m_tty, UV_TTY_MODE_NORMAL);
}

rxs::ConsoleLog::~ConsoleLog()
{
    Handle::close(m_tty);
}

void rxs::ConsoleLog::print(uint64_t, int, const char *line, size_t, size_t size, bool colors)
{
    if (!m_tty || Log::isColors() != colors) {
        return;
    }

    fwrite(line, 1, size, stdout);
    fflush(stdout);
}

bool rxs::ConsoleLog::isSupported()
{
    const uv_handle_type type = uv_guess_handle(1);
    return type == UV_TTY || type == UV_NAMED_PIPE;
}
