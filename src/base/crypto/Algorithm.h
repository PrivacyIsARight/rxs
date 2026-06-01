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

#ifndef RXS_ALGORITHM_H
#define RXS_ALGORITHM_H


#include <cstdint>
#include <functional>
#include <limits>
#include <string_view>
#include <vector>


#include "3rdparty/rapidjson/fwd.h"


namespace rxs {


class Algorithm
{
public:
    // Id encoding:
    // 1 byte: family
    // 1 byte: L3 memory as power of 2 (if applicable).
    // 1 byte: L2 memory for RandomX algorithms as power of 2 or 0x00.
    // 1 byte: extra variant (coin) id.
    enum Id : uint32_t {
        INVALID         = 0,
        RX_0            = 0x72151200,   // "rx/0"             RandomX (reference configuration).
        RX_V2           = 0x72151202,   // "rx/2"             RandomX (Monero v2).
        RX_WOW          = 0x72141177,   // "rx/wow"           RandomWOW (Wownero).
        RX_ARQ          = 0x72121061,   // "rx/arq"           RandomARQ (Arqma).
        RX_GRAFT        = 0x72151267,   // "rx/graft"         RandomGRAFT (Graft).
        RX_SFX          = 0x72151273,   // "rx/sfx"           RandomSFX (Safex Cash).
        RX_YADA         = 0x72151279,   // "rx/yada"          RandomYada (YadaCoin).
    };

    enum Family : uint32_t {
        UNKNOWN         = 0,
        RANDOM_X        = 0x72000000,
    };

    static constexpr const char *kINVALID  = "invalid";
    static constexpr const char *kRX       = "rx";
    static constexpr const char *kRX_0     = "rx/0";
    static constexpr const char *kRX_V2    = "rx/2";
    static constexpr const char *kRX_WOW   = "rx/wow";
    static constexpr const char *kRX_ARQ   = "rx/arq";
    static constexpr const char *kRX_GRAFT = "rx/graft";
    static constexpr const char *kRX_SFX   = "rx/sfx";
    static constexpr const char *kRX_YADA  = "rx/yada";

    inline Algorithm() = default;
    inline Algorithm(const char *algo) noexcept : m_id(parse(algo)) {}
    inline Algorithm(std::string_view algo) noexcept : m_id(parse(algo)) {}
    inline Algorithm(Id id) noexcept : m_id(id)                     {}
    Algorithm(const rapidjson::Value &value);
    Algorithm(uint32_t id);

    static inline constexpr size_t l2(Id id) noexcept
    {
        if (family(id) != RANDOM_X) return 0u;
        const uint32_t exp = (static_cast<uint32_t>(id) >> 8u) & 0xffu;
        return exp <= 31u ? (size_t{1u} << exp) : 0u;
    }

    static inline constexpr size_t l3(Id id) noexcept
    {
        if (id == INVALID) return 0u;
        const uint32_t exp = (static_cast<uint32_t>(id) >> 16u) & 0xffu;
        return exp < std::numeric_limits<size_t>::digits ? (size_t{1u} << exp) : 0u;
    }

    static inline constexpr uint32_t family(Id id) noexcept { return static_cast<uint32_t>(id) & 0xff000000u; }

    inline bool isEqual(const Algorithm &other) const noexcept { return m_id == other.m_id; }
    inline bool isValid() const noexcept                       { return nameView() != kINVALID; }
    inline Id id() const noexcept                              { return m_id; }
    inline size_t l2() const noexcept                          { return l2(m_id); }
    inline size_t l3() const noexcept                          { return l3(m_id); }
    inline uint32_t family() const noexcept                    { return family(m_id); }
    inline uint32_t minIntensity() const noexcept              { return 1; }
    inline uint32_t maxIntensity() const noexcept              { return 1; }

    inline bool operator!=(Algorithm::Id id) const noexcept       { return m_id != id; }
    inline bool operator!=(const Algorithm &other) const noexcept { return !isEqual(other); }
    inline bool operator==(Algorithm::Id id) const noexcept       { return m_id == id; }
    inline bool operator==(const Algorithm &other) const noexcept { return isEqual(other); }
    inline operator Algorithm::Id() const noexcept                { return m_id; }

    const char *name() const noexcept;
    std::string_view nameView() const noexcept;
    rapidjson::Value toJSON() const;
    rapidjson::Value toJSON(rapidjson::Document &doc) const;

    static Id parse(const char *name) noexcept;
    static Id parse(std::string_view name) noexcept;
    static size_t count();
    static std::vector<Algorithm> all(const std::function<bool(const Algorithm &algo)> &filter = nullptr);

private:
    Id m_id = INVALID;
};


using Algorithms = std::vector<Algorithm>;


} /* namespace rxs */


#endif /* RXS_ALGORITHM_H */
