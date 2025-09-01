#include <iostream>
#include "yc/bootstrap.hpp"

int main() {
    using namespace yc;

    Date ref{ 2025,9,1 };
    Date start{ 2025,9,3 }; // ������T+2�c�Ɠ������A�����ł͎b��

    // OIS�N�H�[�g�i��j�F1Y, 2Y, 3Y �̃p�[���[�g
    std::vector<OisParQuote> quotes = {
        { Date{2026,9,3}, 0.0110 },
        { Date{2027,9,3}, 0.0125 },
        { Date{2028,9,4}, 0.0135 },
    };

    // �Œ背�b�O���`�i�N1��, 30/360, �x�����OT+2, �Œ�͎x�����j
    FixedLegParams fp;
    fp.notional = 1'000'000;
    fp.freq = Frequency::Annual;
    fp.dc = DayCount::Thirty360US;
    fp.pay = true;
    fp.payment_lag_days = 2;

    // �������b�O���`�i���N, A/360, OIS����, �ϑ����O2��, �x�����OT+2�j
    FloatLegParams lp;
    lp.notional = 1'000'000;
    lp.freq = Frequency::SemiAnnual;
    lp.dc = DayCount::Actual360;
    lp.pay = false;
    lp.style = FloatStyle::CompoundedInArrears;
    lp.observation_lag_days = 2;
    lp.lockout_days = 0;
    lp.payment_lag_days = 2;

    // �Ȑ��� t �v�Z�� A/365F �ɓ���
    DayCount tdc = DayCount::Actual365Fixed;

    DiscountCurve ois = bootstrap_ois_curve(ref, start, quotes, fp, lp, tdc);

    // �`�F�b�N�F��v���_�� DF ���o��
    double t1 = year_fraction(ref, Date{ 2026,9,3 }, tdc);
    double t2 = year_fraction(ref, Date{ 2027,9,3 }, tdc);
    double t3 = year_fraction(ref, Date{ 2028,9,4 }, tdc);
    std::cout << "DF(1Y last pay) = " << ois.df(t1) << "\n";
    std::cout << "DF(2Y last pay) = " << ois.df(t2) << "\n";
    std::cout << "DF(3Y last pay) = " << ois.df(t3) << "\n";
}
