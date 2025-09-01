#include <iostream>
#include "yc/bootstrap.hpp"
#include "yc/instrument.hpp"

int main() {
    using namespace yc;

    Date ref{ 2025,9,1 };
    Date spot{ 2025,9,3 }; // OISの期初（ここではT+2相当とみなす）

    // 1) デポ（ON, TN, 1W）例：A/360、単利
    DepoQuote d_on{ Date{2025,9,2}, Date{2025,9,3}, 0.0100, DayCount::Actual360, 0 }; // ON
    DepoQuote d_tn{ Date{2025,9,3}, Date{2025,9,4}, 0.0105, DayCount::Actual360, 0 }; // TN
    DepoQuote d_1w{ Date{2025,9,3}, Date{2025,9,10}, 0.0110, DayCount::Actual360, 0 }; // 1W

    // 2) FRA相当（1x4 相当=3Mフォワードの簡易例）: start=1M後, end=4M後
    FraQuote fra_1x4{ Date{2025,10,3}, Date{2026,1,5}, 0.0125, DayCount::Actual360 };

    // 3) OIS スワップ（1Y, 2Y）: RFR in-arrears
    OisParQuote o1y{ Date{2026,9,3}, 0.0120 };
    OisParQuote o2y{ Date{2027,9,3}, 0.0130 };

    std::vector<Instrument> insts = { d_on, d_tn, d_1w, fra_1x4, o1y, o2y };

    // OISレッグの雛形
    FixedLegParams fp;
    fp.notional = 1'000'000;
    fp.freq = Frequency::Annual;
    fp.dc = DayCount::Thirty360US;
    fp.pay = true;
    fp.payment_lag_days = 2;

    FloatLegParams lp;
    lp.notional = 1'000'000;
    lp.freq = Frequency::SemiAnnual;
    lp.dc = DayCount::Actual360;
    lp.pay = false;
    lp.style = FloatStyle::CompoundedInArrears;
    lp.observation_lag_days = 2;
    lp.lockout_days = 0;
    lp.payment_lag_days = 2;

    DayCount tdc = DayCount::Actual365Fixed;

    DiscountCurve curve = bootstrap_curve_mixed(ref, spot, insts, fp, lp, tdc);

    // 主要ノードのDFを確認
    auto show = [&](const Date& d) {
        double t = year_fraction(ref, d, tdc);
        std::cout << "DF(" << d.y << "-" << d.m << "-" << d.d << ") = " << curve.df(t) << "\n";
    };
    show(add_days(d_on.end, d_on.payment_lag_days));
    show(fra_1x4.end);
    show(o1y.end);
    show(o2y.end);
}
