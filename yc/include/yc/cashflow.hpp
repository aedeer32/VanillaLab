#pragma once
#include "date.hpp"

namespace yc {

    // PV�v�Z�p�Ɏx�����𕪗�
    struct Cashflow {
        Date start{};     // �A�N���[���J�n
        Date end{};       // �A�N���[���I���i�N�[�|�����Ԗ��j
        Date pay_date{};  // �x�����i= end + payment_lag_days�j
        double accrual{ 0 };
        double rate{ 0 };
        double amount{ 0 }; // �x�����ɂ�������z
    };

} // namespace yc
