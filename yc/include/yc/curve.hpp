#pragma once
#include <vector>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <cmath>

#include "date.hpp"
#include "daycount.hpp"

namespace yc {

    // DF の log-linear 補間を行うシンプルな割引曲線
    class DiscountCurve {
    public:
        // ピラーは (t[年], DF)。t は 0 以上・昇順、DF は (0,1] を仮定
        explicit DiscountCurve(std::vector<std::pair<double, double>> pillars);

        // 任意時点 t[年] の DF を取得（端はフラット外挿：端 DF を返す）
        double df(double t) const;

    private:
        std::vector<std::pair<double, double>> pillars_;
    };

    // 日付ピラー（基準日・DayCount から t に変換）でカーブを構築
    DiscountCurve make_curve_by_dates(
        const Date& ref,
        const std::vector<std::pair<Date, double>>& date_df,
        DayCount dc);

} // namespace yc
