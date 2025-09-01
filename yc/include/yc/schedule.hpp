#pragma once
#include <vector>
#include "date.hpp"
#include "types.hpp"

namespace yc {

    // 単純化: 月足し＋EOM（end-of-month）維持オプション、支払日は期末＝end
    struct Schedule {
        std::vector<Date> start;   // 各クーポンの開始日
        std::vector<Date> end;     // 各クーポンの終了日（支払日）
    };

    bool is_end_of_month(const Date& d);
    Date add_months(const Date& d, int months, bool keep_eom);

    Schedule make_schedule(
        const Date& start,
        const Date& end,
        Frequency freq,
        bool keep_eom = true
    );

} // namespace yc
