#pragma once
#include "date.hpp"

namespace yc {

    // PV計算用に支払日を分離
    struct Cashflow {
        Date start{};     // アクルール開始
        Date end{};       // アクルール終了（クーポン期間末）
        Date pay_date{};  // 支払日（= end + payment_lag_days）
        double accrual{ 0 };
        double rate{ 0 };
        double amount{ 0 }; // 支払日における金額
    };

} // namespace yc
