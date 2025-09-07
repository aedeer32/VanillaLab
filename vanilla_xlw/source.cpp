#include "cppinterface.h"
#include <algorithm>
#include <string>
#include <cctype>

#include "yc/date.hpp"
#include "yc/daycount.hpp"

// ���[�e�B���e�B�FDayCount ��������p�[�X�i�������O��ԂŏՓˉ���j
namespace {
    yc::DayCount to_daycount(std::string s)
    {
        for (auto& c : s)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        if (s == "ACT/360" || s == "A/360")              return yc::DayCount::Actual360;
        if (s == "ACT/365F" || s == "ACT/365" || s == "A/365F")
            return yc::DayCount::Actual365Fixed;
        if (s == "30/360US" || s == "30U/360" || s == "30/360")
            return yc::DayCount::Thirty360US;

        // �f�t�H���g�i�K�v�ɉ����ĕύX�j
        return yc::DayCount::Actual365Fixed;
    }
} // anonymous namespace

double YC_YearFraction(
    int y1, int m1, int d1,
    int y2, int m2, int d2,
    const std::string& dc)
{
    const yc::Date from{ y1, m1, d1 };
    const yc::Date to{ y2, m2, d2 };

    // �� yc:: �𖾎��i���O�Փ˂Ŋ֐��Ƃ��Č����Ȃ�����h�~�j
    return yc::year_fraction(from, to, to_daycount(dc));
}
