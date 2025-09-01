#pragma once
#include "date.hpp"

namespace yc {

	enum class DayCount { Actual360, Actual365Fixed, Thirty360US };

	// [d1, d2] の年率換算。d2<d1 の場合は負の値を返す（対称性確保）。
	double year_fraction(const Date& d1, const Date& d2, DayCount dc);

} // namespace yc
