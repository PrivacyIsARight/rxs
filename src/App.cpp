/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018      Lee Clagett <https://github.com/vtnerd>
 * Copyright 2018-2024 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#include <atomic>
#include <cstdlib>
#include <stdexcept>
#include <uv.h>


#include "App.h"
#include "backend/cpu/Cpu.h"
#include "base/io/Console.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/io/Signals.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "Summary.h"


static rxs::Process *validatedProcess(rxs::Process *process)
{
    if (!process) {
        throw std::invalid_argument("App: process must not be null");
    }
    return process;
}

rxs::App::App(Process *process) : m_controller(std::make_shared<Controller>(validatedProcess(process)))
{
    [[assume(process != nullptr)]];
    auto* const ctrl = m_controller.get();
    [[assume(ctrl != nullptr)]];
}

rxs::App::~App()
{
    Cpu::release();
}


int rxs::App::exec()
{
    auto* const ctrl = m_controller.get();
    [[assume(ctrl != nullptr)]];
    if (!m_controller->isReady()) [[unlikely]] {
        LOG_EMERG("no valid configuration found");

        return 2;
    }

    if (int rc = 0; background(rc)) [[unlikely]] {
        return rc;

    }
    m_signals = std::make_shared<Signals>(this);

    if (const int rc = m_controller->init(); rc != 0) [[unlikely]] {
        return rc;
    }

    if (!m_controller->isBackground()) {
        m_console = std::make_shared<Console>(this);
    }

    Summary::print(m_controller.get());

    if (m_controller->config()->isDryRun()) [[unlikely]] {
        LOG_NOTICE("%s " WHITE_BOLD("OK"), Tags::config());

        return 0;
    }

    m_controller->start();

    if (auto* const loop = uv_default_loop()) [[likely]] {
        const int rc = uv_run(loop, UV_RUN_DEFAULT);
        int closeRes = uv_loop_close(loop);
        if (closeRes == UV_EBUSY) [[unlikely]] {
            (void) uv_run(loop, UV_RUN_NOWAIT);
            closeRes = uv_loop_close(loop);
        }
        if (closeRes != 0) [[unlikely]] {
            LOG_EMERG("uv_loop_close failed: %s", uv_strerror(closeRes));
            return EXIT_FAILURE;
        }

        return rc;
    }
    LOG_EMERG("failed to acquire libuv default loop");
    close();
    return 1;
}


void rxs::App::onConsoleCommand(const char command)
{
    auto* const ctrl = m_controller.get();
    [[assume(ctrl != nullptr)]];

    constexpr char ctrl_c = '\x03';
    if (command == ctrl_c) [[unlikely]] {
        LOG_WARN("%s " SLATE_BOLD("Ctrl+C received, exiting"), Tags::signal());
        close();
    }
    else [[likely]] {
        m_controller->execCommand(command);
    }
}


void rxs::App::onSignal(const int signum)
{
    switch (signum)
    {
    case SIGHUP:
    case SIGTERM:
    case SIGINT:
        close();
        break;

    default:
        break;
    }
}


void rxs::App::close()
{
    if (m_isClosing.exchange(true)) {
        return;
    }
    auto* const ctrl = m_controller.get();
    [[assume(ctrl != nullptr)]];
    m_signals.reset();
    m_console.reset();

    m_controller->stop();

    Log::destroy();
}
