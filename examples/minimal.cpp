#include <iostream>
#include "yc/date.hpp"
#include "yc/daycount.hpp"
#include "yc/compounding.hpp"
#include "yc/discount.hpp"
#include "yc/curve.hpp"

int main() {
    using namespace yc;
    Date ref{ 2025,8,30 };
    Date mat{ 2026,8,30 };

    double t = year_fraction(ref, mat, DayCount::Actual365Fixed);
    std::cout << "t = " << t << "\n";

    double r = 0.0125; // 1.25% (continuous)
    double df = discount_factor_from_zero(r, t, Compounding::continuous());
    std::cout << "DF(1Y) = " << df << "\n";

    DiscountCurve curve({
        {0.0, 1.0},
        {0.5, 0.9950},
        {1.0, 0.9876},
        {2.0, 0.9750}
        });
    std::cout << "DF(0.75Y) = " << curve.df(0.75) << "\n";

    // 日付ピラー版（30/360 US）
    DiscountCurve bydate = make_curve_by_dates(
        ref,
        { {Date{2026,2,28}, 0.992}, {Date{2026,8,31}, 0.985} },
        DayCount::Thirty360US);
    std::cout << "DF(1Y bydate) = " << bydate.df(t) << "\n";

    return 0;
}
