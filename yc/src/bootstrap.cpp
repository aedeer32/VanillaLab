#include "yc/bootstrap.hpp"
#include <cmath>
#include <algorithm>
#include <functional>
#include <limits>

namespace yc {

    // ===== 既存の bisect を使い回し =====
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

    // ノード時点（新DFを置く日）を決める
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
            // 固定レッグで最後の支払日を取得（支払ラグ込み）
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
        // 満期昇順へ
        std::sort(quotes.begin(), quotes.end(),
            [](const OisParQuote& a, const OisParQuote& b) { return a.end < b.end; });

        // カーブのピラー（t, df）
        std::vector<std::pair<double, double>> pillars;
        pillars.emplace_back(0.0, 1.0);

        // 各OISを1本ずつ処理
        for (const auto& q : quotes) {
            // 固定・浮動の雛形から、このクォート専用の固定レッグを作る（固定額は曲線に依らない）
            FixedLegParams fp = fp_tmpl;
            fp.fixed_rate = q.fixed_rate;

            // 支払日：固定/浮動で一致する前提（支払ラグが同じと想定）
            // 先に固定レッグを構築し、最後の支払日を新ノードとする
            FixedLeg fix_leg = FixedLeg::build(start, q.end, fp);
            if (fix_leg.cfs.empty()) { continue; }

            Date last_pay = fix_leg.cfs.back().pay_date;
            double t_new = year_fraction(ref, last_pay, t_dc);

            // f(df_new) を定義：仮の新ノードを含むカーブで NPV=0 になる DF を探す
            auto f = [&](double df_new)->double {
                // 一時カーブ（既知ピラー + 新ノード）
                auto tmp = pillars;
                // 単調性のため、同一tがあれば入替
                if (!tmp.empty() && std::abs(tmp.back().first - t_new) < 1e-14) tmp.back().second = df_new;
                else tmp.emplace_back(t_new, df_new);
                DiscountCurve curve(tmp);

                // 投影＝割引 同一カーブの一般的OIS前提（必要なら分離可）
                auto df = make_df_func(ref, curve, t_dc);

                // 浮動はDF比で「期末複利(in-arrears)」を毎回再構築（曲線依存）
                FloatLeg flt_leg = FloatLeg::build(start, q.end, lp_tmpl, df);

                // スワップ（固定支払い / 浮動受け取り想定）のNPV
                Swap sw{ fix_leg, flt_leg };
                return sw.npv(df); // 0 を狙う
            };

            // ブラケット上限：直前ノードのDF（単調性上、これ以下）
            double prev_df = pillars.back().second;
            double df_solved = bisect_df_for_root(f, t_new, prev_df);

            // 新ノードを確定
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
        // 1) インストゥルメントを「ノード日」昇順に並べ替え
        std::sort(insts.begin(), insts.end(), [&](const Instrument& a, const Instrument& b) {
            Date da = node_date_for_instrument(a, ois_start, fp_tmpl);
        Date db = node_date_for_instrument(b, ois_start, fp_tmpl);
        return da < db;
            });

        // 2) ピラー初期化
        std::vector<std::pair<double, double>> pillars;
        pillars.emplace_back(0.0, 1.0);

        // 3) 逐次解法
        for (const auto& inst : insts) {
            Date node_d = node_date_for_instrument(inst, ois_start, fp_tmpl);
            double t_new = year_fraction(ref, node_d, t_dc);

            auto f = [&](double df_new)->double {
                // 暫定カーブ
                auto tmp = pillars;
                if (std::abs(tmp.back().first - t_new) < 1e-14) tmp.back().second = df_new;
                else tmp.emplace_back(t_new, df_new);
                DiscountCurve curve(tmp);
                auto df = make_df_func(ref, curve, t_dc);

                // インストゥルメントの「0になるべき方程式値」を返す
                return std::visit([&](auto&& q)->double {
                    using T = std::decay_t<decltype(q)>;
                if constexpr (std::is_same_v<T, DepoQuote>) {
                    // DF(S) - (1 + r α) DF(E_pay) = 0
                    double a = year_fraction(q.start, q.end, q.dc);
                    Date pay = add_days(q.end, q.payment_lag_days);
                    return df(q.start) - (1.0 + q.rate * a) * df(pay);
                }
                else if constexpr (std::is_same_v<T, FraQuote>) {
                    // F(S,E) - r = 0,  F=(DF(S)/DF(E)-1)/α
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
                    return sw.npv(df); // 0 を狙う
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

    // ヘルパー: プロジェクション側の「未知DFを置くノード日」決定
    static Date proj_node_date(const Instrument& inst, const Date& start_for_swaps) {
        return std::visit([&](auto&& q)->Date {
            using T = std::decay_t<decltype(q)>;
        if constexpr (std::is_same_v<T, DepoQuote>) {
            // プロジェクション曲線でもデポは E（支払ラグなし前提。必要ならq.payment_lag_daysを追加で）
            return q.end;
        }
        else if constexpr (std::is_same_v<T, FraQuote>) {
            return q.end; // FRAはEに新ノード
        }
        else if constexpr (std::is_same_v<T, IborSwapQuote>) {
            // 浮動の最後の期末（＝満期）。ペイメントラグは割引側だけに影響。
            return q.end;
        }
        else {
            // OISはここでは扱わない
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
        // 並べ替え：新ノード日（Eや最後の期末）昇順
        std::sort(proj_insts.begin(), proj_insts.end(),
            [&](const Instrument& a, const Instrument& b) {
                return proj_node_date(a, start) < proj_node_date(b, start);
            });

        // プロジェクション・カーブのピラー
        std::vector<std::pair<double, double>> pillars;
        pillars.emplace_back(0.0, 1.0);

        // 割引用 DF 関数（固定）
        auto disc_df = make_df_func(ref, discount_curve, t_dc);

        for (const auto& inst : proj_insts) {
            Date node_d = proj_node_date(inst, start);
            double t_new = year_fraction(ref, node_d, t_dc);

            auto f = [&](double df_new)->double {
                // 暫定プロジェクションカーブ
                auto tmp = pillars;
                if (std::abs(tmp.back().first - t_new) < 1e-14) tmp.back().second = df_new;
                else tmp.emplace_back(t_new, df_new);
                DiscountCurve proj_curve(tmp);
                auto proj_df = make_df_func(ref, proj_curve, t_dc);

                return std::visit([&](auto&& q)->double {
                    using T = std::decay_t<decltype(q)>;
                if constexpr (std::is_same_v<T, DepoQuote>) {
                    // DF_proj(S) - (1 + r*α) * DF_proj(E) = 0 （投影の短端アンカーとして採用）
                    double a = year_fraction(q.start, q.end, q.dc);
                    return proj_df(q.start) - (1.0 + q.rate * a) * proj_df(q.end);
                }
                else if constexpr (std::is_same_v<T, FraQuote>) {
                    // (DF_proj(S)/DF_proj(E) - 1)/α - r = 0
                    double a = year_fraction(q.start, q.end, q.dc);
                    if (a <= 0.0) return 0.0;
                    double F = (proj_df(q.start) / proj_df(q.end) - 1.0) / a;
                    return F - q.rate;
                }
                else if constexpr (std::is_same_v<T, IborSwapQuote>) {
                    // 固定/浮動（IBOR Simple）でスワップ組成。PVは割引=OIS、フォワード=proj。
                    FixedLeg fix = FixedLeg::build(start, q.end, ibor_fix_tmpl);
                    FloatLegParams flp = ibor_float_tmpl;
                    flp.style = FloatStyle::Simple; // 念のため
                    FloatLeg flt = FloatLeg::build(start, q.end, flp, proj_df);

                    // 固定レッグのクーポン率をクォートで上書き（額は固定率×accrual）
                    for (auto& cf : fix.cfs) {
                        cf.rate = q.fixed_rate;
                        cf.amount = fix.params.notional * cf.rate * cf.accrual;
                    }

                    Swap sw{ fix, flt };
                    return sw.npv(disc_df); // 0 を狙う
                }
                else {
                    // OISはここでは解かない
                    return 0.0;
                }
                    }, inst);
            };

            double prev_df = pillars.back().second;
            double solved = /* 既存の二分法を再利用 */[&]() {
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
        // 1) OISで割引カーブ
        DiscountCurve disc = bootstrap_ois_curve(ref, ois_start, ois_quotes, ois_fix_tmpl, ois_float_tmpl, t_dc);
        // 2) デポ/FRA/IBORスワップで投影カーブ（割引はdisc）
        DiscountCurve proj = bootstrap_projection_curve(ref, ibor_start, disc, std::move(proj_insts),
            ibor_fix_tmpl, ibor_float_tmpl, t_dc);
        return { disc, proj };
    }

} // namespace yc
