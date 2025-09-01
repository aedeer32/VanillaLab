#pragma once
#include <vector>
#include "date.hpp"
#include "types.hpp"

namespace yc {

    // �P����: �������{EOM�iend-of-month�j�ێ��I�v�V�����A�x�����͊�����end
    struct Schedule {
        std::vector<Date> start;   // �e�N�[�|���̊J�n��
        std::vector<Date> end;     // �e�N�[�|���̏I�����i�x�����j
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
