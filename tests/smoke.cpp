#include <cassert>
#include "yc/date.hpp"
#include "yc/daycount.hpp"
#include "yc/compounding.hpp"
#include "yc/discount.hpp"
#include "yc/curve.hpp"

int main() {
    using namespace yc;
    Date d1{ 2025,1,1 }, d2{ 2026,1,1 };
    double t = year_fraction(d1, d2, DayCount::Actual365Fixed);
    assert(t > 0.999 && t < 1.001);

    double df = discount_factor_from_zero(0.02, 1.0, Compounding::continuous());
    assert(df > 0.98 && df < 0.99);

    DiscountCurve c({ {0.0,1.0},{1.0,0.98} });
    double mid = c.df(0.5);
    assert(mid < 1.0 && mid > 0.98);

    return 0;
}
