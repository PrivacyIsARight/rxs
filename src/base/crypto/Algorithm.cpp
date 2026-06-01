/* XMRig
 * Copyright (c) 2018      Lee Clagett <https://github.com/vtnerd>
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

#include "base/crypto/Algorithm.h"
#include "3rdparty/rapidjson/document.h"


#include <algorithm>
#include <array>
#include <cassert>
#include <ranges>


namespace rxs {


struct NameEntry {
    Algorithm::Id    id;
    std::string_view name;
};

static constexpr std::array<NameEntry, 7> kNames {{
    { Algorithm::RX_ARQ,   Algorithm::kRX_ARQ   },
    { Algorithm::RX_WOW,   Algorithm::kRX_WOW   },
    { Algorithm::RX_0,     Algorithm::kRX_0     },
    { Algorithm::RX_V2,    Algorithm::kRX_V2    },
    { Algorithm::RX_GRAFT, Algorithm::kRX_GRAFT },
    { Algorithm::RX_SFX,   Algorithm::kRX_SFX   },
    { Algorithm::RX_YADA,  Algorithm::kRX_YADA  },
}};

struct AliasEntry {
    std::string_view alias;
    Algorithm::Id    id;
};

static constexpr std::array<AliasEntry, 14> kAliases {{
    { "randomarq",   Algorithm::RX_ARQ   },
    { "randomgraft", Algorithm::RX_GRAFT },
    { "randomsfx",   Algorithm::RX_SFX   },
    { "randomwow",   Algorithm::RX_WOW   },
    { "randomx",     Algorithm::RX_0     },
    { "randomyada",  Algorithm::RX_YADA  },
    { "rx",          Algorithm::RX_0     },
    { "rx/0",        Algorithm::RX_0     },
    { "rx/2",        Algorithm::RX_V2    },
    { "rx/arq",      Algorithm::RX_ARQ   },
    { "rx/graft",    Algorithm::RX_GRAFT },
    { "rx/sfx",      Algorithm::RX_SFX   },
    { "rx/wow",      Algorithm::RX_WOW   },
    { "rx/yada",     Algorithm::RX_YADA  },
}};

static_assert([] {
    for (size_t i = 1; i < kNames.size(); ++i)
        if (kNames[i].id <= kNames[i-1].id) return false;
    return true;
}(), "kNames must be sorted by id");

static_assert([] {
    for (size_t i = 1; i < kAliases.size(); ++i)
        if (kAliases[i].alias <= kAliases[i-1].alias) return false;
    return true;
}(), "kAliases must be sorted lexicographically");

static constexpr size_t kMaxAliasLen = [] {
    size_t max = 0;
    for (const auto &e : kAliases)
        if (e.alias.size() > max) max = e.alias.size();
    return max;
}();


static constexpr unsigned char ascii_lower_u(char c) noexcept
{
    const auto u = static_cast<unsigned char>(c);
    return (u >= 'A' && u <= 'Z') ? static_cast<unsigned char>(u | 0x20u) : u;
}


static constexpr int ci_compare(std::string_view a, std::string_view b) noexcept
{
    const size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        const int d = static_cast<int>(ascii_lower_u(a[i]))
                    - static_cast<int>(static_cast<unsigned char>(b[i]));
        if (d != 0) return d;
    }
    if (a.size() < b.size()) return -1;
    if (a.size() > b.size()) return  1;
    return 0;
}


Algorithm::Algorithm(const rapidjson::Value &value) : m_id(value.IsString() ? parse(std::string_view(value.GetString(), value.GetStringLength())) : INVALID)
{
}


Algorithm::Algorithm(uint32_t id) : m_id(static_cast<Id>(id))
{
    assert(isValid());
}


std::string_view Algorithm::nameView() const noexcept
{
    const auto it = std::lower_bound(kNames.begin(), kNames.end(), m_id,
        [](const NameEntry &e, Id v) noexcept { return e.id < v; });

    if (it != kNames.end() && it->id == m_id) {
        return it->name;
    }

    return kINVALID;
}


const char *Algorithm::name() const noexcept
{
    return nameView().data();
}


rapidjson::Value Algorithm::toJSON() const
{
    return rapidjson::Value(rapidjson::StringRef(name()));
}


rapidjson::Value Algorithm::toJSON(rapidjson::Document &doc) const
{
    return rapidjson::Value(name(), doc.GetAllocator());
}


Algorithm::Id Algorithm::parse(std::string_view name) noexcept
{
    if (name.size() < 2 || name.size() > kMaxAliasLen) return INVALID;

    size_t lo = 0, hi = kAliases.size();
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2u;
        const int cmp = ci_compare(name, kAliases[mid].alias);
        if (cmp == 0) return kAliases[mid].id;
        if (cmp  < 0) hi = mid;
        else          lo = mid + 1u;
    }
    return INVALID;
}


Algorithm::Id Algorithm::parse(const char *name) noexcept
{
    if (!name) return INVALID;
    return parse(std::string_view(name));
}


size_t Algorithm::count()
{
    return kNames.size();
}


std::vector<Algorithm> Algorithm::all(const std::function<bool(const Algorithm &algo)> &filter)
{
    std::vector<Algorithm> out;
    out.reserve(kNames.size());

    std::ranges::for_each(kNames, [&](const NameEntry &entry) {
        Algorithm algo(entry.id);
        if (!filter || filter(algo)) {
            out.emplace_back(algo);
        }
    });

    return out;
}


} /* namespace rxs */
