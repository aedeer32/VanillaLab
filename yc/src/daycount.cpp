#include "yc/daycount.hpp"
#include <algorithm>
#include <stdexcept>

namespace yc {

    static double yf_act_over(const Date& d1, const Date& d2, double denom) {
        double days = static_cast<double>(days_between(d1, d2));
        return days / denom;
    }

    double year_fraction(const Date& d1, const Date& d2, DayCount dc) {
        if (d2 < d1) return -year_fraction(d2, d1, dc);

        switch (dc) {
        case DayCount::Actual360:
            return yf_act_over(d1, d2, 360.0);
        case DayCount::Actual365Fixed:
            return yf_act_over(d1, d2, 365.0);
        case DayCount::Thirty360US: {
            // 30/360 (US) ‹K–ñ
            int d1d = d1.d == 31 ? 30 : d1.d;
            int d2d = (d2.d == 31 && d1d == 30) ? 30 : d2.d;
            int days = 360 * (d2.y - d1.y) + 30 * (d2.m - d1.m) + (d2d - d1d);
            return static_cast<double>(days) / 360.0;
        }
        default:
            throw std::runtime_error("unknown day count");
        }
    }

} // namespace yc
