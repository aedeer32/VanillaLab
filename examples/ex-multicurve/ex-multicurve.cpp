#include <iostream>
#include "yc/bootstrap.hpp"
#include "yc/instrument.hpp"

int main() {
    using namespace yc;

    Date ref{ 2025,9,1 };
    DayCount tdc = DayCount::Actual365Fixed;

    // --- OIS（割引） ---
    Date ois_start{ 2025,9,3 };
    std::vector<OisParQuote> ois_quotes = {
        { Date{2026,9,3}, 0.0110 },
        { Date{2027,9,3}, 0.0125 },
        { Date{2028,9,3}, 0.0135 },
    };

    FixedLegParams ois_fix; ois_fix.notional = 1e6; ois_fix.freq = Frequency::Annual; ois_fix.dc = DayCount::Thirty360US; ois_fix.pay = true; ois_fix.payment_lag_days = 2;
    FloatLegParams ois_flt; ois_flt.notional = 1e6; ois_flt.freq = Frequency::SemiAnnual; ois_flt.dc = DayCount::Actual360; ois_flt.pay = false; ois_flt.style = FloatStyle::CompoundedInArrears; ois_flt.observation_lag_days = 2; ois_flt.payment_lag_days = 2;

    // --- IBOR（投影） ---
    Date ibor_start{ 2025,9,3 };
    std::vector<Instrument> proj_insts = {
        DepoQuote{ Date{2025,9,3}, Date{2025,9,10}, 0.0110, DayCount::Actual360, 0 },   // 1W
        FraQuote { Date{2025,10,3}, Date{2026,1,3},  0.0120, DayCount::Actual360 },     // 1x4
        IborSwapQuote{ Date{2027,9,3}, 0.0140 },                                         // 2Y
        IborSwapQuote{ Date{2028,9,3}, 0.0150 }                                          // 3Y
    };

    FixedLegParams ibor_fix; ibor_fix.notional = 1e6; ibor_fix.freq = Frequency::Annual; ibor_fix.dc = DayCount::Thirty360US; ibor_fix.pay = true; ibor_fix.payment_lag_days = 2;
    FloatLegParams ibor_flt; ibor_flt.notional = 1e6; ibor_flt.freq = Frequency::Quarterly; ibor_flt.dc = DayCount::Actual360; ibor_flt.pay = false; ibor_flt.style = FloatStyle::Simple; ibor_flt.payment_lag_days = 2;

    MultiCurve mc = bootstrap_multi_curve(ref, ois_start, ois_quotes, ois_fix, ois_flt,
        ibor_start, proj_insts, ibor_fix, ibor_flt, tdc);

    auto show = [&](const char* tag, const DiscountCurve& c, Date d) {
        double t = year_fraction(ref, d, tdc);
        std::cout << tag << " DF(" << d.y << "-" << d.m << "-" << d.d << ") = " << c.df(t) << "\n";
    };

    show("OIS ", mc.discount, Date{ 2027,9,3 });
    show("IBOR", mc.projection, Date{ 2027,9,3 });
}
