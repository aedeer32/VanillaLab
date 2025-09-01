#pragma once
#include <cstdint>
#include <string>
#include <stdexcept>
#include <algorithm>

namespace yc {

    struct Date {
        int y, m, d;

        static Date from_string(const std::string& s) {
            if (s.size() != 10 || s[4] != '-' || s[7] != '-') throw std::invalid_argument("bad date");
            return { std::stoi(s.substr(0,4)), std::stoi(s.substr(5,2)), std::stoi(s.substr(8,2)) };
        }
    };

    inline bool operator<(const Date& a, const Date& b) {
        if (a.y != b.y) return a.y < b.y;
        if (a.m != b.m) return a.m < b.m;
        return a.d < b.d;
    }

    namespace detail {
        inline int64_t days_from_civil(int y, unsigned m, unsigned d) {
            y -= m <= 2;
            const int era = (y >= 0 ? y : y - 399) / 400;
            const unsigned yoe = static_cast<unsigned>(y - era * 400);
            const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
            const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + yoe / 400 + doy;
            return era * 146097 + static_cast<int>(doe) - 719468; // 1970-01-01 → 0
        }

        // 逆変換：通日→(y,m,d)
        inline Date civil_from_days(int64_t z1970) {
            int64_t z = z1970 + 719468;
            const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
            const unsigned doe = static_cast<unsigned>(z - era * 146097);
            const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
            int y = static_cast<int>(yoe) + static_cast<int>(era) * 400;
            const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100 + yoe / 400);
            const unsigned mp = (5 * doy + 2) / 153;
            const unsigned d = doy - (153 * mp + 2) / 5 + 1;
            const unsigned m = mp + (mp < 10 ? 3 : -9);
            y += (m <= 2);
            return { y, static_cast<int>(m), static_cast<int>(d) };
        }
    } // namespace detail

    inline int64_t days_between(const Date& a, const Date& b) {
        return detail::days_from_civil(b.y, b.m, b.d) - detail::days_from_civil(a.y, a.m, a.d);
    }

    // ★ 追加：日単位のシフト（カレンダー日）
    inline Date add_days(const Date& d, int n) {
        auto z = detail::days_from_civil(d.y, d.m, d.d);
        return detail::civil_from_days(z + n);
    }

} // namespace yc
