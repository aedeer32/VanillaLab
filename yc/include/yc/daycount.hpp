#pragma once
#include "date.hpp"

namespace yc {

	enum class DayCount { Actual360, Actual365Fixed, Thirty360US };

	// [d1, d2] �̔N�����Z�Bd2<d1 �̏ꍇ�͕��̒l��Ԃ��i�Ώ̐��m�ہj�B
	double year_fraction(const Date& d1, const Date& d2, DayCount dc);

} // namespace yc
