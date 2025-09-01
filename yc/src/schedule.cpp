#include "yc/schedule.hpp"
#include <algorithm>

namespace yc {

    static int days_in_month(int y, int m) {
        static const int mdays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
        int d = mdays[m - 1];
        bool leap = ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
        if (m == 2 && leap) d = 29;
        return d;
    }

    bool is_end_of_month(const Date& d) {
        return d.d == days_in_month(d.y, d.m);
    }

    Date add_months(const Date& d, int months, bool keep_eom) {
        int y = d.y, m = d.m + months, day = d.d;
        while (m > 12) { m -= 12; ++y; }
        while (m < 1) { m += 12; --y; }

        if (keep_eom && is_end_of_month(d)) {
            day = days_in_month(y, m); // EOM �ێ�
        }
        else {
            day = std::min(day, days_in_month(y, m));
        }
        return { y, m, day };
    }

    Schedule make_schedule(const Date& s, const Date& e, Frequency f, bool keep_eom) {
        Schedule sch;
        int step = months_per_period(f);

        Date d0 = s;
        Date next = d0;
        // �O�i�����i�Ō�͕K�� e �ŃN���[�Y�j
        while (true) {
            next = add_months(d0, step, keep_eom);
            if ((e < next) || (next < d0)) { // ����t�]������I��
                break;
            }
            if (e < next) next = e;
            if (!(next < d0)) {
                sch.start.push_back(d0);
                if (next < e) {
                    sch.end.push_back(next);
                    d0 = next;
                    if (!(d0 < e) && !(e < d0)) break;
                }
                else {
                    sch.end.push_back(e);
                    break;
                }
            }
            if (!(next < e)) {
                // �ŏI���
                if (sch.start.empty() || !(sch.end.back().y == e.y && sch.end.back().m == e.m && sch.end.back().d == e.d)) {
                    sch.start.push_back(d0);
                    sch.end.push_back(e);
                }
                break;
            }
        }

        // ���� s==e�i�[�����ԁj�ȂǁA��Ȃ�1���������
        if (sch.start.empty()) {
            sch.start.push_back(s);
            sch.end.push_back(e);
        }
        return sch;
    }

} // namespace yc
