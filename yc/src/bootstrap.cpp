#include "yc/bootstrap.hpp"
#include <cmath>
#include <algorithm>
#include <functional>
#include <limits>

namespace yc {

    // ===== ������ bisect ���g���� =====
    static double bisect_df_for_root(
        const std::function<double(double)>& f, double t_new, double prev_df)
    {
        const double df_hi0 = std::min(prev_df, 1.0);
        const double r_high = 0.50;
        double df_lo = std::exp(-r_high * std::max(1e-12, t_new));
        double df_hi = df_hi0;
        double f_lo = f(df_lo), f_hi = f(df_hi);
        for (int k = 0; k < 8 && f_lo * f_hi>0.0; ++k) {
            df_lo = std::max(1e-300, df_lo * 0.2);
            f_lo = f(df_lo);
            df_hi = std::min(1.0, df_hi * 1.000001);
            f_hi = f(df_hi);
        }
        if (f_lo * f_hi > 0.0) return (std::abs(f_hi) < std::abs(f_lo)) ? df_hi : df_lo;
        for (int it = 0; it < 120; ++it) {
            double mid = 0.5 * (df_lo + df_hi);
            double fm = f(mid);
            if (std::abs(fm) < 1e-12 * (1.0 + std::abs(f_hi) + std::abs(f_lo))) return mid;
            if (f_lo * fm <= 0.0) { df_hi = mid; f_hi = fm; }
            else { df_lo = mid; f_lo = fm; }
            if (std::abs(df_hi - df_lo) < 1e-14 * std::max(1.0, df_hi)) return 0.5 * (df_lo + df_hi);
        }
        return 0.5 * (df_lo + df_hi);
    }

    // �m�[�h���_�i�VDF��u�����j�����߂�
    static Date node_date_for_instrument(
        const Instrument& inst, const Date& ois_start, const FixedLegParams& fp_tmpl)
    {
        return std::visit([&](auto&& q)->Date {
            using T = std::decay_t<decltype(q)>;
        if constexpr (std::is_same_v<T, DepoQuote>) {
            return add_days(q.end, q.payment_lag_days);
        }
        else if constexpr (std::is_same_v<T, FraQuote>) {
            return q.end;
        }
        else { // OisParQuote
            // �Œ背�b�O�ōŌ�̎x�������擾�i�x�����O���݁j
            FixedLegParams fp = fp_tmpl;
            fp.fixed_rate = q.fixed_rate;
            auto fix = FixedLeg::build(ois_start, q.end, fp);
            return fix.cfs.back().pay_date;
        }
            }, inst);
    }

    DiscountCurve bootstrap_ois_curve(
        const Date& ref,
        const Date& start,
        std::vector<OisParQuote> quotes,
        FixedLegParams fp_tmpl,
        FloatLegParams lp_tmpl,
        DayCount t_dc
    ) {
        // ����������
        std::sort(quotes.begin(), quotes.end(),
            [](const OisParQuote& a, const OisParQuote& b) { return a.end < b.end; });

        // �J�[�u�̃s���[�it, df�j
        std::vector<std::pair<double, double>> pillars;
        pillars.emplace_back(0.0, 1.0);

        // �eOIS��1�{������
        for (const auto& q : quotes) {
            // �Œ�E�����̐��`����A���̃N�H�[�g��p�̌Œ背�b�O�����i�Œ�z�͋Ȑ��Ɉ˂�Ȃ��j
            FixedLegParams fp = fp_tmpl;
            fp.fixed_rate = q.fixed_rate;

            // �x�����F�Œ�/�����ň�v����O��i�x�����O�������Ƒz��j
            // ��ɌŒ背�b�O���\�z���A�Ō�̎x������V�m�[�h�Ƃ���
            FixedLeg fix_leg = FixedLeg::build(start, q.end, fp);
            if (fix_leg.cfs.empty()) { continue; }

            Date last_pay = fix_leg.cfs.back().pay_date;
            double t_new = year_fraction(ref, last_pay, t_dc);

            // f(df_new) ���`�F���̐V�m�[�h���܂ރJ�[�u�� NPV=0 �ɂȂ� DF ��T��
            auto f = [&](double df_new)->double {
                // �ꎞ�J�[�u�i���m�s���[ + �V�m�[�h�j
                auto tmp = pillars;
                // �P�����̂��߁A����t������Γ���
                if (!tmp.empty() && std::abs(tmp.back().first - t_new) < 1e-14) tmp.back().second = df_new;
                else tmp.emplace_back(t_new, df_new);
                DiscountCurve curve(tmp);

                // ���e������ ����J�[�u�̈�ʓIOIS�O��i�K�v�Ȃ番���j
                auto df = make_df_func(ref, curve, t_dc);

                // ������DF��Łu��������(in-arrears)�v�𖈉�č\�z�i�Ȑ��ˑ��j
                FloatLeg flt_leg = FloatLeg::build(start, q.end, lp_tmpl, df);

                // �X���b�v�i�Œ�x���� / �����󂯎��z��j��NPV
                Swap sw{ fix_leg, flt_leg };
                return sw.npv(df); // 0 ��_��
            };

            // �u���P�b�g����F���O�m�[�h��DF�i�P������A����ȉ��j
            double prev_df = pillars.back().second;
            double df_solved = bisect_df_for_root(f, t_new, prev_df);

            // �V�m�[�h���m��
            if (!pillars.empty() && std::abs(pillars.back().first - t_new) < 1e-14) {
                pillars.back().second = df_solved;
            }
            else {
                pillars.emplace_back(t_new, df_solved);
            }
        }

        return DiscountCurve{ pillars };
    }

    DiscountCurve bootstrap_curve_mixed(
        const Date& ref,
        const Date& ois_start,
        std::vector<Instrument> insts,
        FixedLegParams fp_tmpl,
        FloatLegParams lp_tmpl,
        DayCount t_dc
    ) {
        // 1) �C���X�g�D�������g���u�m�[�h���v�����ɕ��בւ�
        std::sort(insts.begin(), insts.end(), [&](const Instrument& a, const Instrument& b) {
            Date da = node_date_for_instrument(a, ois_start, fp_tmpl);
        Date db = node_date_for_instrument(b, ois_start, fp_tmpl);
        return da < db;
            });

        // 2) �s���[������
        std::vector<std::pair<double, double>> pillars;
        pillars.emplace_back(0.0, 1.0);

        // 3) ������@
        for (const auto& inst : insts) {
            Date node_d = node_date_for_instrument(inst, ois_start, fp_tmpl);
            double t_new = year_fraction(ref, node_d, t_dc);

            auto f = [&](double df_new)->double {
                // �b��J�[�u
                auto tmp = pillars;
                if (std::abs(tmp.back().first - t_new) < 1e-14) tmp.back().second = df_new;
                else tmp.emplace_back(t_new, df_new);
                DiscountCurve curve(tmp);
                auto df = make_df_func(ref, curve, t_dc);

                // �C���X�g�D�������g�́u0�ɂȂ�ׂ��������l�v��Ԃ�
                return std::visit([&](auto&& q)->double {
                    using T = std::decay_t<decltype(q)>;
                if constexpr (std::is_same_v<T, DepoQuote>) {
                    // DF(S) - (1 + r ��) DF(E_pay) = 0
                    double a = year_fraction(q.start, q.end, q.dc);
                    Date pay = add_days(q.end, q.payment_lag_days);
                    return df(q.start) - (1.0 + q.rate * a) * df(pay);
                }
                else if constexpr (std::is_same_v<T, FraQuote>) {
                    // F(S,E) - r = 0,  F=(DF(S)/DF(E)-1)/��
                    double a = year_fraction(q.start, q.end, q.dc);
                    if (a <= 0.0) return 0.0;
                    double F = (df(q.start) / df(q.end) - 1.0) / a;
                    return F - q.rate;
                }
                else { // OisParQuote
                    FixedLegParams fp = fp_tmpl; fp.fixed_rate = q.fixed_rate;
                    FixedLeg fix = FixedLeg::build(ois_start, q.end, fp);
                    FloatLeg flt = FloatLeg::build(ois_start, q.end, lp_tmpl, df);
                    Swap sw{ fix, flt };
                    return sw.npv(df); // 0 ��_��
                }
                    }, inst);
            };

            double prev_df = pillars.back().second;
            double solved = bisect_df_for_root(f, t_new, prev_df);

            if (std::abs(pillars.back().first - t_new) < 1e-14) pillars.back().second = solved;
            else pillars.emplace_back(t_new, solved);
        }

        return DiscountCurve{ pillars };
    }

    // �w���p�[: �v���W�F�N�V�������́u���mDF��u���m�[�h���v����
    static Date proj_node_date(const Instrument& inst, const Date& start_for_swaps) {
        return std::visit([&](auto&& q)->Date {
            using T = std::decay_t<decltype(q)>;
        if constexpr (std::is_same_v<T, DepoQuote>) {
            // �v���W�F�N�V�����Ȑ��ł��f�|�� E�i�x�����O�Ȃ��O��B�K�v�Ȃ�q.payment_lag_days��ǉ��Łj
            return q.end;
        }
        else if constexpr (std::is_same_v<T, FraQuote>) {
            return q.end; // FRA��E�ɐV�m�[�h
        }
        else if constexpr (std::is_same_v<T, IborSwapQuote>) {
            // �����̍Ō�̊����i�������j�B�y�C�����g���O�͊����������ɉe���B
            return q.end;
        }
        else {
            // OIS�͂����ł͈���Ȃ�
            return start_for_swaps;
        }
            }, inst);
    }

    DiscountCurve bootstrap_projection_curve(
        const Date& ref,
        const Date& start,
        const DiscountCurve& discount_curve,
        std::vector<Instrument> proj_insts,
        FixedLegParams ibor_fix_tmpl,
        FloatLegParams ibor_float_tmpl,
        DayCount t_dc
    ) {
        // ���בւ��F�V�m�[�h���iE��Ō�̊����j����
        std::sort(proj_insts.begin(), proj_insts.end(),
            [&](const Instrument& a, const Instrument& b) {
                return proj_node_date(a, start) < proj_node_date(b, start);
            });

        // �v���W�F�N�V�����E�J�[�u�̃s���[
        std::vector<std::pair<double, double>> pillars;
        pillars.emplace_back(0.0, 1.0);

        // �����p DF �֐��i�Œ�j
        auto disc_df = make_df_func(ref, discount_curve, t_dc);

        for (const auto& inst : proj_insts) {
            Date node_d = proj_node_date(inst, start);
            double t_new = year_fraction(ref, node_d, t_dc);

            auto f = [&](double df_new)->double {
                // �b��v���W�F�N�V�����J�[�u
                auto tmp = pillars;
                if (std::abs(tmp.back().first - t_new) < 1e-14) tmp.back().second = df_new;
                else tmp.emplace_back(t_new, df_new);
                DiscountCurve proj_curve(tmp);
                auto proj_df = make_df_func(ref, proj_curve, t_dc);

                return std::visit([&](auto&& q)->double {
                    using T = std::decay_t<decltype(q)>;
                if constexpr (std::is_same_v<T, DepoQuote>) {
                    // DF_proj(S) - (1 + r*��) * DF_proj(E) = 0 �i���e�̒Z�[�A���J�[�Ƃ��č̗p�j
                    double a = year_fraction(q.start, q.end, q.dc);
                    return proj_df(q.start) - (1.0 + q.rate * a) * proj_df(q.end);
                }
                else if constexpr (std::is_same_v<T, FraQuote>) {
                    // (DF_proj(S)/DF_proj(E) - 1)/�� - r = 0
                    double a = year_fraction(q.start, q.end, q.dc);
                    if (a <= 0.0) return 0.0;
                    double F = (proj_df(q.start) / proj_df(q.end) - 1.0) / a;
                    return F - q.rate;
                }
                else if constexpr (std::is_same_v<T, IborSwapQuote>) {
                    // �Œ�/�����iIBOR Simple�j�ŃX���b�v�g���BPV�͊���=OIS�A�t�H���[�h=proj�B
                    FixedLeg fix = FixedLeg::build(start, q.end, ibor_fix_tmpl);
                    FloatLegParams flp = ibor_float_tmpl;
                    flp.style = FloatStyle::Simple; // �O�̂���
                    FloatLeg flt = FloatLeg::build(start, q.end, flp, proj_df);

                    // �Œ背�b�O�̃N�[�|�������N�H�[�g�ŏ㏑���i�z�͌Œ藦�~accrual�j
                    for (auto& cf : fix.cfs) {
                        cf.rate = q.fixed_rate;
                        cf.amount = fix.params.notional * cf.rate * cf.accrual;
                    }

                    Swap sw{ fix, flt };
                    return sw.npv(disc_df); // 0 ��_��
                }
                else {
                    // OIS�͂����ł͉����Ȃ�
                    return 0.0;
                }
                    }, inst);
            };

            double prev_df = pillars.back().second;
            double solved = /* �����̓񕪖@���ė��p */[&]() {
                const double df_hi0 = std::min(prev_df, 1.0);
                const double r_high = 0.50;
                double df_lo = std::exp(-r_high * std::max(1e-12, t_new));
                double df_hi = df_hi0;
                double f_lo = f(df_lo), f_hi = f(df_hi);
                for (int k = 0; k < 8 && f_lo * f_hi>0.0; ++k) {
                    df_lo = std::max(1e-300, df_lo * 0.2);
                    f_lo = f(df_lo);
                    df_hi = std::min(1.0, df_hi * 1.000001);
                    f_hi = f(df_hi);
                }
                if (f_lo * f_hi > 0.0) return (std::abs(f_hi) < std::abs(f_lo)) ? df_hi : df_lo;
                for (int it = 0; it < 120; ++it) {
                    double mid = 0.5 * (df_lo + df_hi);
                    double fm = f(mid);
                    if (std::abs(fm) < 1e-12 * (1.0 + std::abs(f_hi) + std::abs(f_lo))) return mid;
                    if (f_lo * fm <= 0.0) { df_hi = mid; f_hi = fm; }
                    else { df_lo = mid; f_lo = fm; }
                    if (std::abs(df_hi - df_lo) < 1e-14 * std::max(1.0, df_hi)) return 0.5 * (df_lo + df_hi);
                }
                return 0.5 * (df_lo + df_hi);
            }();

            if (std::abs(pillars.back().first - t_new) < 1e-14) pillars.back().second = solved;
            else pillars.emplace_back(t_new, solved);
        }

        return DiscountCurve{ pillars };
    }

    MultiCurve bootstrap_multi_curve(
        const Date& ref,
        const Date& ois_start,
        const std::vector<OisParQuote>& ois_quotes,
        FixedLegParams ois_fix_tmpl, FloatLegParams ois_float_tmpl,
        const Date& ibor_start,
        std::vector<Instrument> proj_insts,
        FixedLegParams ibor_fix_tmpl, FloatLegParams ibor_float_tmpl,
        DayCount t_dc
    ) {
        // 1) OIS�Ŋ����J�[�u
        DiscountCurve disc = bootstrap_ois_curve(ref, ois_start, ois_quotes, ois_fix_tmpl, ois_float_tmpl, t_dc);
        // 2) �f�|/FRA/IBOR�X���b�v�œ��e�J�[�u�i������disc�j
        DiscountCurve proj = bootstrap_projection_curve(ref, ibor_start, disc, std::move(proj_insts),
            ibor_fix_tmpl, ibor_float_tmpl, t_dc);
        return { disc, proj };
    }

} // namespace yc
