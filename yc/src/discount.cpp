#include "yc/discount.hpp"

namespace yc {

    double discount_factor_from_zero(double r, double t, const Compounding& c) {
        if (t < 0) throw std::invalid_argument("t must be >= 0");
        if (t == 0) return 1.0;

        switch (c.kind) {
        case CompKind::Simple:
            return 1.0 / (1.0 + r * t);
        case CompKind::Compounded:
            if (c.m <= 0) throw std::invalid_argument("compounding m must be > 0");
            return std::pow(1.0 + r / c.m, -c.m * t);
        case CompKind::Continuous:
            return std::exp(-r * t);
        default:
            throw std::runtime_error("unknown compounding");
        }
    }

    double zero_rate_from_df(double df, double t, const Compounding& c) {
        if (t <= 0) throw std::invalid_argument("t must be > 0");
        if (!(df > 0.0 && df <= 1.0)) throw std::invalid_argument("df in (0,1]");

        switch (c.kind) {
        case CompKind::Simple:
            return (1.0 / df - 1.0) / t;
        case CompKind::Compounded:
            return c.m * (std::pow(df, -1.0 / (c.m * t)) - 1.0);
        case CompKind::Continuous:
            return -std::log(df) / t;
        default:
            throw std::runtime_error("unknown compounding");
        }
    }

} // namespace yc
