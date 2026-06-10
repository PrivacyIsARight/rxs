/* XMRig
 * Copyright (c) 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2020 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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


#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>


#include "crypto/common/Assembly.h"
#include "3rdparty/rapidjson/document.h"


namespace rxs {


static constexpr std::array<std::string_view, Assembly::MAX> asmNames = {{
    "none",
    "auto",
    "intel",
    "ryzen",
    "bulldozer"
}};


static bool iequal(std::string_view a, std::string_view b)
{
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(), [](unsigned char x, unsigned char y) {
               return std::tolower(x) == std::tolower(y);
           });
}


} /* namespace rxs */


rxs::Assembly::Id rxs::Assembly::parse(const char *assembly, Id defaultValue)
{
    static_assert(asmNames.size() == MAX, "asmNames size mismatch");

    if (assembly == nullptr) {
        return defaultValue;
    }

    for (size_t i = 0; i < asmNames.size(); i++) {
        if (iequal(assembly, asmNames[i])) {
            return static_cast<Id>(i);
        }
    }

    return defaultValue;
}


rxs::Assembly::Id rxs::Assembly::parse(const rapidjson::Value &value, Id defaultValue)
{
    if (value.IsBool()) {
        return value.GetBool() ? AUTO : NONE;
    }

    if (value.IsString()) {
        return parse(value.GetString(), defaultValue);
    }

    return defaultValue;
}


const char *rxs::Assembly::toString() const
{
    if (m_id < NONE || m_id >= MAX) {
        return "invalid";
    }

    return asmNames[static_cast<size_t>(m_id)].data();
}


rapidjson::Value rxs::Assembly::toJSON() const
{
    using namespace rapidjson;

    if (m_id == NONE) {
        return Value(false);
    }

    if (m_id == AUTO) {
        return Value(true);
    }

    return Value(StringRef(toString()));
}
