#include <iostream>
#include "yc/bootstrap.hpp"

int main() {
    using namespace yc;

    Date ref{ 2025,9,1 };
    Date start{ 2025,9,3 }; // 実務はT+2営業日だが、ここでは暫定

    // OISクォート（例）：1Y, 2Y, 3Y のパーレート
    std::vector<OisParQuote> quotes = {
        { Date{2026,9,3}, 0.0110 },
        { Date{2027,9,3}, 0.0125 },
        { Date{2028,9,4}, 0.0135 },
    };

    // 固定レッグ雛形（年1回, 30/360, 支払ラグT+2, 固定は支払い）
    FixedLegParams fp;
    fp.notional = 1'000'000;
    fp.freq = Frequency::Annual;
    fp.dc = DayCount::Thirty360US;
    fp.pay = true;
    fp.payment_lag_days = 2;

    // 浮動レッグ雛形（半年, A/360, OIS複利, 観測ラグ2日, 支払ラグT+2）
    FloatLegParams lp;
    lp.notional = 1'000'000;
    lp.freq = Frequency::SemiAnnual;
    lp.dc = DayCount::Actual360;
    lp.pay = false;
    lp.style = FloatStyle::CompoundedInArrears;
    lp.observation_lag_days = 2;
    lp.lockout_days = 0;
    lp.payment_lag_days = 2;

    // 曲線の t 計算は A/365F に統一
    DayCount tdc = DayCount::Actual365Fixed;

    DiscountCurve ois = bootstrap_ois_curve(ref, start, quotes, fp, lp, tdc);

    // チェック：主要時点の DF を出力
    double t1 = year_fraction(ref, Date{ 2026,9,3 }, tdc);
    double t2 = year_fraction(ref, Date{ 2027,9,3 }, tdc);
    double t3 = year_fraction(ref, Date{ 2028,9,4 }, tdc);
    std::cout << "DF(1Y last pay) = " << ois.df(t1) << "\n";
    std::cout << "DF(2Y last pay) = " << ois.df(t2) << "\n";
    std::cout << "DF(3Y last pay) = " << ois.df(t3) << "\n";
}
