/* XMRig
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

#include "base/net/stratum/BaseClient.h"
#include "3rdparty/fmt/core.h"
#include "3rdparty/rapidjson/document.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/kernel/interfaces/IClientListener.h"
#include "base/net/stratum/SubmitResult.h"


namespace rxs {


std::atomic<int64_t> BaseClient::m_sequence{1};


} /* namespace rxs */


rxs::BaseClient::BaseClient(int id, IClientListener *listener) :
    m_listener(listener),
    m_id(id)
{
}


void rxs::BaseClient::setPool(const Pool &pool)
{
    if (!pool.isValid()) {
        return;
    }

    m_pool     = pool;
    m_user     = m_pool.user();
    m_password = m_pool.password();
    m_rigId    = m_pool.rigId();

    m_tag.clear();
    fmt::format_to(std::back_inserter(m_tag), "{} " CYAN_BOLD("{}"), Tags::network(), m_pool.url().data());
}


bool rxs::BaseClient::handleResponse(int64_t id, const rapidjson::Value &result, const rapidjson::Value &error)
{
    if (id <= 1) {
        return false;
    }

    const auto it = m_callbacks.find(id);
    if (it != m_callbacks.end()) {
        const uint64_t elapsed = Chrono::steadyMSecs() - it->second.ts;

        if (error.IsObject()) {
            it->second.callback(error, false, elapsed);
        }
        else {
            it->second.callback(result, true, elapsed);
        }

        m_callbacks.erase(it);

        return true;
    }

    return false;
}


bool rxs::BaseClient::handleSubmitResponse(int64_t id, const char *error)
{
    const auto it = m_results.find(id);
    if (it != m_results.end()) {
        it->second.done();
        m_listener->onResultAccepted(this, it->second, error);
        m_results.erase(it);

        return true;
    }

    return false;
}
