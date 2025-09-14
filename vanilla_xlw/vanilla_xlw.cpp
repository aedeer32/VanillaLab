// vanilla_xlw.cpp
//
// Excel(XLW) ����ĂԔ������b�p�[�� 1 �t�@�C���ɏW��B
// - �ˑ��Fyc/ (date.hpp, daycount.hpp, curve.hpp �Ȃ�)
// - ���j�FExcel ���́u������E���l�EExcel �V���A�����t(long)�v�݂̂��󂯎��A
//         �ϊ��� yc API �Ăяo�� �� ���ʂ� double/long �ŕԂ��B
// - ���ӁFDayCount �Ȃǂ̗񋓂� Excel ���ɘI�o�������Astring �Ŏ󂯂ăp�[�X�B
//         �i_xlwcppinterface.cpp �� �ginvalid literal suffix 'ACT'�h ������j
//
// �����F�֐��̐錾�i�G�N�X�|�[�g�w��� <xlw:...> �^�O�j�͓����̃w�b�_
//       VanillaXlw.h �ɒu���čĐ�������^�p�B
//       �����ł͎����݂̂�񋟁B

#include <stdexcept>
#include <string>
#include <cstdint>
#include <cmath>
#include <limits>

#include "yc/date.hpp"
#include "yc/daycount.hpp"
// �K�v�ɉ����āF#include "yc/curve.hpp"
// �K�v�ɉ����āF#include "yc/discount.hpp"

using std::string;

namespace xlw_bridge {

    //------------------------------------------------------------
    // Excel 1900 �V���A�����t ���� yc::Date �ϊ��w���p
    //------------------------------------------------------------
    //
    // Excel(1900) �́u1900�N�����邤�N�ƌ�F�v������j�I�d�l�������A
    // �V���A�� 60 �����݁i1900-02-29�j�B�����ł͋ƊE�ň�ʓI�Ȉȉ��̈����F
    //   - ����F1899-12-31 �� 1 �Ƃ���i���� 1900-01-01 �� 1�j
    //   - �V���A�� 60 �̓_�~�[�Ƃ��Ēʉ߂����A���t���ł� 1900-03-01 �ւ��炷
    //
    // �Q�l�F�����̎����́u�V���A�� >= 61 �̂Ƃ� 1 �������v�����ŕ␳�B
    //
    static inline yc::Date from_excel_serial(long serial)
    {
        if (serial <= 0) {
            throw std::invalid_argument("Excel serial must be positive.");
        }
        // 1899-12-31 �� day 0 �ƍl����
        // 1900-01-01 = 1, ..., 1900-03-01 = 61�i������ 60 �̓_�~�[�j
        long days = serial;

        // 1900-02-29 �̃_�~�[�iserial=60�j�ȍ~�� 1 �������Ď����t�ɍ��킹��
        if (serial >= 61) {
            days -= 1;
        }
        else if (serial == 60) {
            // �_�~�[���t�F����I�� 1900-03-01 �ƌ��Ȃ�
            return yc::Date{ 1900, 3, 1 };
        }

        // �����E�X���Ȃǂ��g�킸�ȈՉ��Z�Fyc::Date �ɓ����Z�w���p���Ȃ����
        // ��U y/m/d �ɕ������Čv�Z�c�����Ȍ��̂��߁Ayc ���� day-shift ������΂���𗘗p�B
        // �����ł͎��O�ŃO���S���I����Z�������i�ŏ����j�B
        auto is_leap = [](int y) {
            return (y % 400 == 0) || ((y % 4 == 0) && (y % 100 != 0));
        };
        int y = 1899, m = 12, d = 31;
        static const int mdays_norm[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

        // days �񂾂� 1 �����i�߂�i���\���K�v�Ȃ烆���E�X�����Z�ɒu���j
        for (long i = 0; i < days; ++i) {
            int md = mdays_norm[m - 1] + ((m == 2 && is_leap(y)) ? 1 : 0);
            if (d < md) { ++d; }
            else {
                d = 1;
                if (m < 12) { ++m; }
                else { m = 1; ++y; }
            }
        }
        return yc::Date{ y, m, d };
    }

    static inline long to_excel_serial(const yc::Date& date)
    {
        // 1899-12-31 �� 0 �Ƃ��A�ȍ~�̌o�ߓ��� + leap-bug �␳
        auto is_leap = [](int y) {
            return (y % 400 == 0) || ((y % 4 == 0) && (y % 100 != 0));
        };
        auto days_in_month = [&](int y, int m) {
            static const int mdays_norm[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
            return mdays_norm[m - 1] + ((m == 2 && is_leap(y)) ? 1 : 0);
        };

        int y = 1899, m = 12, d = 31;
        long serial = 0;

        // 1899-12-31 ���� date �܂� 1 �����i�߂�i�������͕K�v�Ȃ����j
        while (true) {
            if (y == date.y && m == date.m && d == date.d) break;
            int md = days_in_month(y, m);
            if (d < md) { ++d; }
            else {
                d = 1;
                if (m < 12) { ++m; }
                else { m = 1; ++y; }
            }
            ++serial;
        }

        // 1900-02-28(=59) �̎��Ƀ_�~�[ 60(1900-02-29) ��}�����邽�߁A
        // ����ȍ~�� +1 ���� Excel �̃V���A���ցB
        // �����t 1900-03-01 �ȍ~ �� +1
        if (serial >= 60) serial += 1;
        return serial;
    }

    //------------------------------------------------------------
    // DayCount ������ �� yc::DayCount
    //------------------------------------------------------------
    static inline yc::DayCount parse_dc(const std::string& s_raw)
    {
        // �啶�����ƃX���b�V��/�󔒂̐��K���i�ŏ����j
        string s;
        s.reserve(s_raw.size());
        for (char c : s_raw) {
            if (c == ' ' || c == '-') continue;
            s.push_back(static_cast<char>(::toupper(static_cast<unsigned char>(c))));
        }

        // �悭�g���ʖ����z��
        if (s == "ACT360" || s == "ACT/360" || s == "ACTUAL360" || s == "A360")
            return yc::DayCount::Actual360;
        if (s == "ACT365" || s == "ACT/365" || s == "ACT/365F" || s == "ACTUAL365F" || s == "A365F")
            return yc::DayCount::Actual365Fixed;
        if (s == "30U360" || s == "30/360US" || s == "30_360US" || s == "30E360US" || s == "30US360")
            return yc::DayCount::Thirty360US;

        throw std::invalid_argument("Unsupported DayCount: " + s_raw);
    }

} // namespace xlw_bridge


//------------------------------------------------------------
// �������� Excel �Ɍ��J����֐��̎���
//------------------------------------------------------------
//
// �d�v�FXLW �̎��������́u�錾�iVanillaXlw.h�j�v���Q�Ƃ��܂��B
//       - �֐����E�������E�^�͐錾�ƈ�v�����Ă�������
//       - DayCount �� string �Ŏ󂯎��A�{�t�@�C���� parse_dc ���܂�
//
// �񋟊֐��i�ŏ��̍ŏ��Z�b�g�j
//   1) YearFraction(start_serial, end_serial, dc_str) -> double
//   2) DateAddDays(excel_serial, days) -> excel_serial
//   3) DateToYMD(excel_serial) -> ������ "YYYY-MM-DD"�i���֊֐��F���ؗp�j
//
// �����ǉ��i��j
//   - BuildDiscountCurve(name, ref_serial, knot_serials[], dfs[])
//   - DF(name, pay_serial, dc_str)
//   - ZeroRate(name, pay_serial, dc_str, compounding, m)
//
// �܂��� �g�����֐��n�h ����^�p���A���ɃJ�[�u�o�^�n�𑫂��̂����S�ł��B

extern "C" {

    // Year fraction between two Excel serial dates, given a daycount string.
    double YearFraction(long start_excel_serial, long end_excel_serial, const char* dc_cstr)
    {
        try {
            if (dc_cstr == nullptr) throw std::invalid_argument("DayCount string is null.");
            std::string dc_str(dc_cstr);

            const yc::Date d1 = xlw_bridge::from_excel_serial(start_excel_serial);
            const yc::Date d2 = xlw_bridge::from_excel_serial(end_excel_serial);
            const yc::DayCount dc = xlw_bridge::parse_dc(dc_str);

            return yc::year_fraction(d1, d2, dc);
        }
        catch (const std::exception& e) {
            // Excel �ɂ� NaN ��Ԃ��A�ʃZ���ŃG���[���b�Z�[�W�\���ł���悤�ɂ���^�p������
            // �i�K�v�Ȃ烍�O�֏o���j
            (void)e;
            return std::numeric_limits<double>::quiet_NaN();
        }
    }

    // Add N days to an Excel serial date and return Excel serial date.
    long DateAddDays(long excel_serial, int days)
    {
        try {
            yc::Date d = xlw_bridge::from_excel_serial(excel_serial);

            // �ŏ����̉��Z�i�������͌���j�Fdays>=0 �œ��X�C���N�������g
            auto is_leap = [](int y) {
                return (y % 400 == 0) || ((y % 4 == 0) && (y % 100 != 0));
            };
            auto days_in_month = [&](int y, int m) {
                static const int mdays_norm[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
                return mdays_norm[m - 1] + ((m == 2 && is_leap(y)) ? 1 : 0);
            };

            int y = d.y, m = d.m, dd = d.d;

            if (days >= 0) {
                for (int i = 0; i < days; ++i) {
                    int md = days_in_month(y, m);
                    if (dd < md) ++dd;
                    else { dd = 1; if (m < 12) ++m; else { m = 1; ++y; } }
                }
            }
            else {
                for (int i = 0; i < -days; ++i) {
                    if (dd > 1) --dd;
                    else { if (m > 1) --m; else { m = 12; --y; } dd = days_in_month(y, m); }
                }
            }

            return xlw_bridge::to_excel_serial(yc::Date{ y, m, dd });
        }
        catch (...) {
            return -1; // Excel ���� #VALUE! �����ɂ��Ă��炤
        }
    }

    // Convenience: convert an Excel serial to "YYYY-MM-DD" text (for debugging).
    // Excel ����̌Ăяo���F=DateToYMD(A1)
    __declspec(dllexport) const char* DateToYMD(long excel_serial)
    {
        // ���ӁF�ÓI�o�b�t�@�̓X���b�h�Z�[�t�ł͂Ȃ��BXLL �͒ʏ�V���O���X���b�h����
        // �K�v�ɉ����� TLS �Ɉڍs�B
        static thread_local std::string buf;
        try {
            yc::Date d = xlw_bridge::from_excel_serial(excel_serial);
            char tmp[32];
            std::snprintf(tmp, sizeof(tmp), "%04d-%02d-%02d", d.y, d.m, d.d);
            buf = tmp;
            return buf.c_str();
        }
        catch (...) {
            buf = "ERR";
            return buf.c_str();
        }
    }

} // extern "C"


//------------------------------------------------------------
// �����g���̃X�P���g���i�R�����g�A�E�g��j
//------------------------------------------------------------
/*

// ��F�����J�[�u�� Excel ����o�^�iname �ŕێ��j
#include <vector>
#include <unordered_map>

namespace {
    struct CurveHolder {
        yc::Date ref;
        // ���ۂ� yc::DiscountCurve �̃R���X�g���N�^�ɍ��킹�ĕێ��`�𒲐�
        // �����ł͐����p�_�~�[�F
        // yc::DiscountCurve curve;
    };

    std::unordered_map<std::string, CurveHolder> g_curves;
}

extern "C" {

// Excel �z��̎󂯓n���� XLW �̌^�ɍ��킹��K�v����B
// �����ł͐����p�Ƀ|�C���^�{���������A���ۂ� xlw::XlfOper / FP12 ���𗘗p�B
double BuildDiscountCurve(
    const char* name_cstr,
    long ref_excel_serial,
    const double* knot_serials, int knot_len,
    const double* dfs,          int dfs_len,
    const char* dc_cstr)
{
    // �c������ yc �̎��R���X�g���N�^�ɍ��킹�� nodes ��g�ݗ��Ăĕێ�
    return 1.0; // �����Ȃ� 1.0 �Ȃ�
}

double DF(const char* name_cstr, long pay_excel_serial, const char* dc_cstr)
{
    // g_curves[name].curve.df(t) ��Ԃ���
    return std::numeric_limits<double>::quiet_NaN();
}

} // extern "C"

*/

// �ǉ��FYC_YearFraction �̎��́i_xlwcppinterface.cpp ��������Ăԁj
double YC_YearFraction(int y1, int m1, int d1,
    int y2, int m2, int d2,
    const std::string& dc_str)
{
    try {
        const yc::Date d_start{ y1, m1, d1 };
        const yc::Date d_end{ y2, m2, d2 };
        const yc::DayCount dc = xlw_bridge::parse_dc(dc_str);
        return yc::year_fraction(d_start, d_end, dc);
    }
    catch (...) {
        return std::numeric_limits<double>::quiet_NaN();
    }
}

// �V�K�iExcel�V���A���Łj
double YC_YearFractionSerial(int start_excel_serial,
    int end_excel_serial,
    const std::string& dc_str)
{
    try {
        const yc::Date d1 = xlw_bridge::from_excel_serial(start_excel_serial);
        const yc::Date d2 = xlw_bridge::from_excel_serial(end_excel_serial);
        const yc::DayCount dc = xlw_bridge::parse_dc(dc_str);
        return yc::year_fraction(d1, d2, dc);
    }
    catch (...) {
        return std::numeric_limits<double>::quiet_NaN();
    }
}
