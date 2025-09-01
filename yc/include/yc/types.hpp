#pragma once
#include <stdexcept>

namespace yc {

    enum class Frequency { Annual = 1, SemiAnnual = 2, Quarterly = 4, Monthly = 12 };

    inline int months_per_period(Frequency f) {
        int n = static_cast<int>(f);
        if (n <= 0) throw std::invalid_argument("bad frequency");
        return 12 / n; // e.g., Quarterly(4) -> 3 months
    }

    enum class FloatStyle {
        Simple,               // 期中一定の単純フォワード（従来）
        CompoundedInArrears   // RFRの期末複利（SOFR/TONA等）
    };

} // namespace yc
