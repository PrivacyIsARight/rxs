/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2020 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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


#include "net/JobResults.h"
#include "backend/common/Tags.h"
#include "base/io/Async.h"
#include "base/io/log/Log.h"
#include "base/kernel/interfaces/IAsyncListener.h"
#include "base/tools/Object.h"
#include "net/interfaces/IJobResultListener.h"
#include "net/JobResult.h"


#include "crypto/randomx/randomx.h"
#include "crypto/rx/Rx.h"
#include "crypto/rx/RxVm.h"
#include "crypto/common/Assembly.h"


#include <cassert>
#include <memory>
#include <mutex>
#include <vector>
#include <uv.h>


namespace rxs {


class JobResultsPrivate : public IAsyncListener
{
public:
    RXS_DISABLE_COPY_MOVE_DEFAULT(JobResultsPrivate)

    inline JobResultsPrivate(IJobResultListener *listener) :
        m_listener(listener)
    {
        m_async = std::make_shared<Async>(this);
    }


    ~JobResultsPrivate() override = default;


    inline void submit(const JobResult &result)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_results.push_back(result);

        m_async->send();
    }


protected:
    inline void onAsync() override  { submit(); }


private:
    inline void submit()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_processingResults.swap(m_results);
        }

        for (const auto &result : m_processingResults) {
            m_listener->onJobResult(result);
        }

        m_processingResults.clear();
    }

    IJobResultListener *m_listener;
    std::vector<JobResult> m_results;
    std::vector<JobResult> m_processingResults;
    std::mutex m_mutex;
    std::shared_ptr<Async> m_async;
};


static JobResultsPrivate *handler = nullptr;


} // namespace rxs


void rxs::JobResults::done(const Job &job)
{
    submit(JobResult(job));
}


void rxs::JobResults::setListener(IJobResultListener *listener)
{
    assert(handler == nullptr);

    handler = new JobResultsPrivate(listener);
}


void rxs::JobResults::stop()
{
    assert(handler != nullptr);

    delete handler;

    handler = nullptr;
}


void rxs::JobResults::submit(const Job &job, uint32_t nonce, const uint8_t *result)
{
    submit(JobResult(job, nonce, result));
}


void rxs::JobResults::submit(const Job& job, uint32_t nonce, const uint8_t* result, const uint8_t* extra_data)
{
    submit(JobResult(job, nonce, result, nullptr, nullptr, extra_data));
}


void rxs::JobResults::submit(const JobResult &result)
{
    assert(handler != nullptr);

    if (handler) {
        handler->submit(result);
    }
}
