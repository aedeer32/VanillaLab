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

    // �Œ背�b�O
    struct FixedLegParams {
        double   notional{ 1.0 };
        double   fixed_rate{ 0.0 };
        Frequency freq{ Frequency::Annual };
        DayCount dc{ DayCount::Actual365Fixed };
        bool     pay{ true };
        int      payment_lag_days{ 0 };    // �� �x�����O
    };

    // �������b�O�iRFR�Ή��j
    struct FloatLegParams {
        double   notional{ 1.0 };
        double   spread{ 0.0 };
        Frequency freq{ Frequency::SemiAnnual };
        DayCount dc{ DayCount::Actual360 };
        bool     pay{ false };
        FloatStyle style{ FloatStyle::Simple };

        // �� �ϑ��n�p�����[�^�i�J�����_�[���ŊȈՑΉ��j
        int observation_lag_days{ 0 };   // ���̊ϑ������ۂ��Ɓg�O�Ɂh���炷
        int lookback_days{ 0 };          // �ǉ��̑O�|���i= ��ƍ��Z���Ĉ����j
        int lockout_days{ 0 };           // �ϑ����I�[�̌Œ�����i�ȈՁF�I�[������ɑO�ɋl�߂�j

        int payment_lag_days{ 0 };       // �x�����O
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
