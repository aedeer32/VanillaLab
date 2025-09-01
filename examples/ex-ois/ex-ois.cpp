#include <iostream>
#include "yc/date.hpp"
#include "yc/daycount.hpp"
#include "yc/curve.hpp"
#include "yc/types.hpp"
#include "yc/swap.hpp"

int main() {
    using namespace yc;

    Date ref{ 2025,9,1 };
    // 適当な OIS カーブ（DFピラー）
    DiscountCurve ois({
        {0.0, 1.0},
        {0.5, 0.9978},
        {1.0, 0.9950},
        {2.0, 0.9890},
        {5.0, 0.9700}
        });
    auto df = make_df_func(ref, ois, DayCount::Actual365Fixed); // 投影=割引 同一（単一カーブ）

    // 2Y OIS: 固定(年1回, 30/360) vs SOFR/TONA(半年, A/360)
    Date start{ 2025,9,3 }, end{ 2027,9,3 };

    FixedLegParams fp;
    fp.notional = 10'000'000;
    fp.fixed_rate = 0.012;                 // 1.2%仮定
    fp.freq = Frequency::Annual;
    fp.dc = DayCount::Thirty360US;
    fp.pay = true;                  // 固定支払い
    fp.payment_lag_days = 2;      // 固定側もT+2に合わせる

    FloatLegParams lp;
    lp.notional = 10'000'000;
    lp.spread = 0.0000;                // SOFR/TONA スプレッド
    lp.freq = Frequency::SemiAnnual; // 半年クーポン
    lp.dc = DayCount::Actual360;   // RFR標準
    lp.pay = false;                 // 浮動受け取り
    lp.style = FloatStyle::CompoundedInArrears; // ★ OIS 複利
    // ★ 追加
    lp.observation_lag_days = 2;  // 例：2日観測シフト（lookbackも加えたいならここに+）
    lp.lockout_days = 0;  // 例：lockoutなし（2日にしたければ 2）
    lp.payment_lag_days = 2;  // 例：T+2支払

    Swap sw{
        FixedLeg::build(start, end, fp),
        FloatLeg::build(start, end, lp, df)
    };

    double pv = sw.npv(df);
    double par = sw.par_rate(df);

    std::cout << "OIS NPV = " << pv << "\n";
    std::cout << "OIS Par = " << par << "\n";

    // 簡易検証：浮動PV ≈ N*(P(T0)-P(Tn)) + N*s*Σ alpha_i P(T_{i+1})
    double check = 0.0;
    {
        // 手計算で検算（s=0 のときは単に N*(P0-Pn)）
        // ここでは s=0 なので N*(P0-Pn) だけ
        double p0 = df(start), pn = df(end);
        check = lp.notional * (p0 - pn);
        std::cout << "Float PV check ~ " << check << " vs model "
            << sw.floating.pv(df) << "\n";
    }

    // 追加検証
    double float_pv_check = 0.0;
    for (size_t i = 0; i < sw.floating.cfs.size(); ++i) {
        const auto& cf = sw.floating.cfs[i];
        // 観測窓
        yc::Date obs_start = yc::add_days(cf.start, -(lp.observation_lag_days + lp.lookback_days));
        yc::Date obs_end = yc::add_days(cf.end, -lp.observation_lag_days);
        if (lp.lockout_days > 0) obs_end = yc::add_days(obs_end, -lp.lockout_days);

        double gross = df(obs_start) / df(obs_end) - 1.0;   // 複利リターン
        double amt = lp.notional * (gross + lp.spread * cf.accrual);
        float_pv_check += amt * df(cf.pay_date);            // 支払日で割引
    }

    std::cout << "New Float PV check ~ " << float_pv_check << " vs model " << sw.floating.pv(df) << "\n";
    // ここで float_pv_check と sw.floating.pv(df) は一致するはず
}
