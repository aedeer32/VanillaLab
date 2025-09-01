#include "yc/swap.hpp"
#include "yc/discount.hpp"

namespace yc {

    DfFunc make_df_func(const Date& ref, const DiscountCurve& curve, DayCount dc_for_t) {
        return [&ref, &curve, dc_for_t](const Date& d) {
            double t = year_fraction(ref, d, dc_for_t);
            return curve.df(t);
        };
    }

    // ---- FixedLeg ----
    FixedLeg FixedLeg::build(const Date& start, const Date& end, const FixedLegParams& p) {
        FixedLeg leg; leg.params = p;
        auto sch = make_schedule(start, end, p.freq, /*keep_eom=*/true);
        leg.cfs.reserve(sch.end.size());

        for (size_t i = 0; i < sch.end.size(); ++i) {
            Cashflow cf;
            cf.start = sch.start[i];
            cf.end = sch.end[i];
            cf.pay_date = add_days(cf.end, p.payment_lag_days);
            cf.accrual = year_fraction(cf.start, cf.end, p.dc);
            cf.rate = p.fixed_rate;
            cf.amount = p.notional * cf.rate * cf.accrual; // 支払日における額
            leg.cfs.push_back(cf);
        }
        return leg;
    }

    double FixedLeg::pv(const DfFunc& disc_df) const {
        double v = 0.0; for (const auto& cf : cfs) v += cf.amount * disc_df(cf.pay_date); return v;
    }

    double FixedLeg::annuity(const DfFunc& disc_df) const {
        double a = 0.0; for (const auto& cf : cfs) a += params.notional * cf.accrual * disc_df(cf.pay_date); return a;
    }

    // ---- FloatLeg ----
    static double forward_simple(const DfFunc& df, const Date& d1, const Date& d2, DayCount dc) {
        double a = year_fraction(d1, d2, dc); if (a <= 0.0) return 0.0;
        double df1 = df(d1), df2 = df(d2);
        return (df1 / df2 - 1.0) / a;
    }

    FloatLeg FloatLeg::build(const Date& start, const Date& end, const FloatLegParams& p, const DfFunc& proj_df) {
        FloatLeg leg; leg.params = p;
        auto sch = make_schedule(start, end, p.freq, /*keep_eom=*/true);
        leg.cfs.reserve(sch.end.size());

        const int obs_shift = p.observation_lag_days + p.lookback_days;

        for (size_t i = 0; i < sch.end.size(); ++i) {
            Cashflow cf;
            cf.start = sch.start[i];
            cf.end = sch.end[i];
            cf.pay_date = add_days(cf.end, p.payment_lag_days);
            cf.accrual = year_fraction(cf.start, cf.end, p.dc);

            if (p.style == FloatStyle::Simple) {
                // 従来IBOR型（期中一定）
                double fwd = forward_simple(proj_df, cf.start, cf.end, p.dc);
                cf.rate = fwd + p.spread;
                cf.amount = p.notional * cf.rate * cf.accrual;
            }
            else {
                // RFR OIS: 複利 in-arrears（観測シフト＋lockout 簡易）
                Date obs_start = add_days(cf.start, -obs_shift);
                Date obs_end = add_days(cf.end, -p.observation_lag_days);
                if (p.lockout_days > 0) {
                    obs_end = add_days(obs_end, -p.lockout_days); // 終端をさらに手前へ
                }
                // 安全対策：期間が潰れたらゼロ
                double gross = 0.0;
                if (!(obs_end < obs_start)) {
                    double df1 = proj_df(obs_start);
                    double df2 = proj_df(obs_end);
                    gross = (df1 / df2 - 1.0); // 複利リターン
                }
                double spread_amt = p.spread * cf.accrual; // スプレッドはアクルールに対して単純加算
                cf.amount = p.notional * (gross + spread_amt);

                // ログや検査用の等価年率（PVには未使用）
                cf.rate = (cf.accrual > 0.0) ? (gross / cf.accrual + p.spread) : 0.0;
            }

            leg.cfs.push_back(cf);
        }
        return leg;
    }

    double FloatLeg::pv(const DfFunc& disc_df) const {
        double v = 0.0; for (const auto& cf : cfs) v += cf.amount * disc_df(cf.pay_date); return v;
    }

} // namespace yc
