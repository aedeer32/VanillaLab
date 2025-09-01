#pragma once
#include <variant>
#include "date.hpp"
#include "daycount.hpp"
#include "types.hpp"
#include "swap.hpp"   // OISのパラメータ（Fixed/Float）にアクセスするため

namespace yc {

    // ---- デポ（単利が普通） ----
    // PV=0条件: DF(0, S) - (1 + r*α) * DF(0, E_pay) = 0
    struct DepoQuote {
        Date start;
        Date end;              // 通常: start < end
        double rate;           // 年率クォート（例: 0.012 = 1.2%）
        DayCount dc;           // 例: Actual360
        int payment_lag_days{ 0 }; // 通常 0（T+0）。必要ならT+2など
    };

    // ---- FRA相当（短期フォワード） ----
    // 条件: F(S,E) = r_quote,  ここで F = (DF(S)/DF(E)-1)/α
    // 伝統的FRAはS決済だが、パリティ上は上の等式だけでDF(E)が決まる
    struct FraQuote {
        Date start;
        Date end;      // 新ノードは E に置く
        double rate;   // 年率クォート
        DayCount dc;   // 例: Actual360
    };

    // ---- OIS（既存） ----
    struct OisParQuote {
        Date end;
        double fixed_rate;
    };

    // ★ 追加: IBORスワップのパーレート
    // 期初はAPI引数で与える（スポットなど）。固定/浮動の頻度やDCはテンプレートで指定。
    struct IborSwapQuote {
        Date end;          // スワップ満期
        double fixed_rate; // 年率（例 0.0125）
    };

    // 混在インストゥルメント
    using Instrument = std::variant<DepoQuote, FraQuote, OisParQuote, IborSwapQuote>;

} // namespace yc
