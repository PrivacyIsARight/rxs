#ifndef RXS_FORMAT_H
#define RXS_FORMAT_H


#include <cstdint>
#include <cstring>
#include <string>


namespace rxs {


class Format
{
public:
    static std::string withCommas(uint64_t n)
    {
        std::string s = std::to_string(n);
        for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) {
            s.insert(static_cast<size_t>(i), ",");
        }
        return s;
    }

    static std::string withCommas(std::string s)
    {
        const size_t decimal       = s.find('.');
        const int    integer_digits = static_cast<int>(decimal == std::string::npos ? s.size() : decimal);

        for (int i = integer_digits - 3; i > 0; i -= 3) {
            s.insert(static_cast<size_t>(i), ",");
        }
        return s;
    }

    static void withCommas(char *buf, size_t size)
    {
        if (!buf || size == 0) return;
        const std::string result = withCommas(std::string(buf));
        strncpy(buf, result.c_str(), size - 1);
        buf[size - 1] = '\0';
    }
};


} // namespace rxs


#endif // RXS_FORMAT_H
