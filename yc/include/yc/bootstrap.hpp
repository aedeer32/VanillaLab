#pragma once
#include <vector>
#include <utility>
#include <algorithm>
#include "date.hpp"
#include "daycount.hpp"
#include "curve.hpp"
#include "swap.hpp"
#include "types.hpp"
#include "instrument.hpp"

namespace yc {

    // OISカーブ・ブートストラップ：
    // - ref        : 曲線の参照日（t=0）
    // - start      : 各OISの期初（単純化：全クォート同一の期初）
    // - quotes     : 満期昇順（昇順にソートされていなければソートします）
    // - fp_tmpl    : 固定レッグの雛形（notional/freq/dc/pay/lag）※fixed_rateはquoteで上書き
    // - lp_tmpl    : 浮動レッグの雛形（SOFR/TONA向けに style=CompoundedInArrears を推奨）
    // - t_dc       : 曲線で t[年] を作るときの DayCount（例 A/365F）
    // 返り値：割引曲線（DFはlog-linear補間）
    // 手順：既知ピラー + 「新満期の最後の支払日DF＝未知数」を使い、NPV=0 を二分法で解く
    DiscountCurve bootstrap_ois_curve(
        const Date& ref,
        const Date& start,
        std::vector<OisParQuote> quotes,
        FixedLegParams fp_tmpl,
        FloatLegParams lp_tmpl,
        DayCount t_dc
    );

    // 新規: デポ/FRA/OIS を混在させたブートストラップ
    // - ref       : カーブの参照日
    // - start     : 各OISの期初（OISのみ使用）
    // - insts     : Depo/FRA/OIS のベクタ（満期順でなくてもOK）
    // - fp_tmpl   : OIS用固定レッグ雛形（fixed_rateはOISクォートで上書き）
    // - lp_tmpl   : OIS用浮動レッグ雛形（RFR複利 in-arrears 推奨）
    // - t_dc      : 年数t計算の規約（例 A/365F）
    // 返り値: 割引曲線（log-linear on DF）
    DiscountCurve bootstrap_curve_mixed(
        const Date& ref,
        const Date& start,
        std::vector<Instrument> insts,
        FixedLegParams fp_tmpl,
        FloatLegParams lp_tmpl,
        DayCount t_dc
    );

    // ★ 新規: 割引(OIS)が既知のもとで、投影カーブ(IBOR)を構築
    DiscountCurve bootstrap_projection_curve(
        const Date& ref,
        const Date& start,                     // FRA/IRSの期初（スポット等）
        const DiscountCurve& discount_curve,   // 既に構築済みOIS
        std::vector<Instrument> proj_insts,    // Depo / FRA / IborSwap のみを想定
        FixedLegParams ibor_fix_tmpl,          // 固定レッグ雛形（freq, dc, pay, lag）
        FloatLegParams ibor_float_tmpl,        // 浮動レッグ雛形（style=Simple を推奨）
        DayCount t_dc                          // t計算（例 A/365F）
    );

    // ★ 便利: OIS(割引) → IBOR(投影) を一括構築
    struct MultiCurve {
        DiscountCurve discount;  // OIS
        DiscountCurve projection; // IBOR
    };

    MultiCurve bootstrap_multi_curve(
        const Date& ref,
        const Date& ois_start,
        const std::vector<OisParQuote>& ois_quotes,
        FixedLegParams ois_fix_tmpl, FloatLegParams ois_float_tmpl,
        const Date& ibor_start,
        std::vector<Instrument> proj_insts,
        FixedLegParams ibor_fix_tmpl, FloatLegParams ibor_float_tmpl,
        DayCount t_dc
    );

} // namespace yc
