#include <iostream>
#include "yc/date.hpp"
#include "yc/daycount.hpp"
#include "yc/curve.hpp"
#include "yc/types.hpp"
#include "yc/schedule.hpp"
#include "yc/swap.hpp"

int main() {
    using namespace yc;

    // �Q�Ɠ��ƒP��J�[�u�i�����ł̓_�~�[�̃s���[�j
    Date ref{ 2025,9,1 };
    DiscountCurve curve({
        {0.0, 1.0},
        {0.5, 0.9975},
        {1.0, 0.9920},
        {2.0, 0.9800},
        {5.0, 0.9400}
        });

    // DF�񋟊֐��i���e=�����𓯈�J�[�u�Ŋȗ��j
    auto df = make_df_func(ref, curve, DayCount::Actual365Fixed);

    // 2�N���X���b�v�i�Œ�N1��E�ϓ����N�j
    Date start{ 2025,9,3 }, end{ 2027,9,3 };

    FixedLegParams fp;
    fp.notional = 1'000'000;
    fp.fixed_rate = 0.012; // 1.2% p.a.
    fp.freq = Frequency::Annual;
    fp.dc = DayCount::Thirty360US;
    fp.pay = true;

    FloatLegParams lp;
    lp.notional = 1'000'000;
    lp.spread = 0.0000;
    lp.freq = Frequency::SemiAnnual;
    lp.dc = DayCount::Actual360;
    lp.pay = false;

    Swap sw{
        FixedLeg::build(start, end, fp),
        FloatLeg::build(start, end, lp, df)
    };

    double pv = sw.npv(df);
    double par = sw.par_rate(df);

    std::cout << "Swap NPV = " << pv << "\n";
    std::cout << "Par rate = " << par << "\n";
}
