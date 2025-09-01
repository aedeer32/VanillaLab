#pragma once
#include <vector>
#include <functional>
#include "types.hpp"
#include "date.hpp"
#include "daycount.hpp"
#include "schedule.hpp"
#include "cashflow.hpp"
#include "curve.hpp"

namespace yc {

    using DfFunc = std::function<double(const Date&)>;
    DfFunc make_df_func(const Date& ref, const DiscountCurve& curve, DayCount dc_for_t);

    // 固定レッグ
    struct FixedLegParams {
        double   notional{ 1.0 };
        double   fixed_rate{ 0.0 };
        Frequency freq{ Frequency::Annual };
        DayCount dc{ DayCount::Actual365Fixed };
        bool     pay{ true };
        int      payment_lag_days{ 0 };    // ★ 支払ラグ
    };

    // 浮動レッグ（RFR対応）
    struct FloatLegParams {
        double   notional{ 1.0 };
        double   spread{ 0.0 };
        Frequency freq{ Frequency::SemiAnnual };
        DayCount dc{ DayCount::Actual360 };
        bool     pay{ false };
        FloatStyle style{ FloatStyle::Simple };

        // ★ 観測系パラメータ（カレンダー日で簡易対応）
        int observation_lag_days{ 0 };   // 期の観測窓を丸ごと“前に”ずらす
        int lookback_days{ 0 };          // 追加の前倒し（= 上と合算して扱う）
        int lockout_days{ 0 };           // 観測窓終端の固定日数（簡易：終端をさらに前に詰める）

        int payment_lag_days{ 0 };       // 支払ラグ
    };

    struct FixedLeg {
        FixedLegParams params;
        std::vector<Cashflow> cfs;

        static FixedLeg build(const Date& start, const Date& end, const FixedLegParams& p);
        double pv(const DfFunc& disc_df) const;
        double annuity(const DfFunc& disc_df) const;
    };

    struct FloatLeg {
        FloatLegParams params;
        std::vector<Cashflow> cfs;

        static FloatLeg build(const Date& start, const Date& end, const FloatLegParams& p, const DfFunc& proj_df);
        double pv(const DfFunc& disc_df) const;
    };

    struct Swap {
        FixedLeg fixed;
        FloatLeg floating;

        double npv(const DfFunc& disc_df) const {
            double sgn_fix = fixed.params.pay ? -1.0 : 1.0;
            double sgn_flt = floating.params.pay ? -1.0 : 1.0;
            return sgn_fix * fixed.pv(disc_df) + sgn_flt * floating.pv(disc_df);
        }
        double par_rate(const DfFunc& disc_df) const {
            double pv_float = floating.pv(disc_df) * (floating.params.pay ? -1.0 : 1.0);
            double ann = fixed.annuity(disc_df);
            return (ann > 0.0) ? pv_float / ann : 0.0;
        }
    };

} // namespace yc
