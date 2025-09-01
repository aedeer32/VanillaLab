#pragma once
#include <vector>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <cmath>

#include "date.hpp"
#include "daycount.hpp"

namespace yc {

    // DF �� log-linear ��Ԃ��s���V���v���Ȋ����Ȑ�
    class DiscountCurve {
    public:
        // �s���[�� (t[�N], DF)�Bt �� 0 �ȏ�E�����ADF �� (0,1] ������
        explicit DiscountCurve(std::vector<std::pair<double, double>> pillars);

        // �C�ӎ��_ t[�N] �� DF ���擾�i�[�̓t���b�g�O�}�F�[ DF ��Ԃ��j
        double df(double t) const;

    private:
        std::vector<std::pair<double, double>> pillars_;
    };

    // ���t�s���[�i����EDayCount ���� t �ɕϊ��j�ŃJ�[�u���\�z
    DiscountCurve make_curve_by_dates(
        const Date& ref,
        const std::vector<std::pair<Date, double>>& date_df,
        DayCount dc);

} // namespace yc
