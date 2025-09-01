#pragma once
#include <stdexcept>
#include <cmath>
#include "compounding.hpp"

namespace yc {

	// �[������ r�E���� t �N�E�����K�� c ���� DF ���Z�o
	double discount_factor_from_zero(double r, double t, const Compounding& c);

	// DF�E���� t �N�E�����K�� c ���� �[���������t�Z
	double zero_rate_from_df(double df, double t, const Compounding& c);

} // namespace yc
