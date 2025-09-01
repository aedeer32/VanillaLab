#include "yc/curve.hpp"

namespace yc {

    DiscountCurve::DiscountCurve(std::vector<std::pair<double, double>> pillars)
        : pillars_(std::move(pillars))
    {
        if (pillars_.empty()) throw std::invalid_argument("empty pillars");
        for (size_t i = 0; i < pillars_.size(); ++i) {
            const double t = pillars_[i].first;
            const double df = pillars_[i].second;
            if (t < 0.0) throw std::invalid_argument("t < 0");
            if (!(df > 0.0 && df <= 1.0)) throw std::invalid_argument("df out of (0,1]");
            if (i > 0 && !(pillars_[i - 1].first <= t)) throw std::invalid_argument("t not sorted");
        }
    }

    double DiscountCurve::df(double t) const {
        if (t <= pillars_.front().first) return pillars_.front().second;
        if (t >= pillars_.back().first)  return pillars_.back().second;

        // upper_bound で右端ピラーを取得
        auto it = std::upper_bound(
            pillars_.begin(), pillars_.end(), t,
            [](double tv, const std::pair<double, double>& p) { return tv < p.first; });

        const double t2 = it->first;
        const double df2 = it->second;
        const double t1 = (it - 1)->first;
        const double df1 = (it - 1)->second;

        const double w = (t - t1) / (t2 - t1);
        const double ln_df = (1.0 - w) * std::log(df1) + w * std::log(df2);
        return std::exp(ln_df);
    }

    DiscountCurve make_curve_by_dates(
        const Date& ref,
        const std::vector<std::pair<Date, double>>& date_df,
        DayCount dc)
    {
        std::vector<std::pair<double, double>> p;
        p.reserve(date_df.size() + 1);
        p.emplace_back(0.0, 1.0);
        for (const auto& dd : date_df) {
            const double t = year_fraction(ref, dd.first, dc);
            p.emplace_back(t, dd.second);
        }
        std::sort(p.begin(), p.end(), [](auto& a, auto& b) { return a.first < b.first; });
        return DiscountCurve{ std::move(p) };
    }

} // namespace yc
