#pragma once
#include <stdexcept>
#include <cmath>
#include "compounding.hpp"

namespace yc {

	// ƒ[ƒ‹à—˜ rEŠÔ t ”NE•¡—˜‹K–ñ c ‚©‚ç DF ‚ğZo
	double discount_factor_from_zero(double r, double t, const Compounding& c);

	// DFEŠÔ t ”NE•¡—˜‹K–ñ c ‚©‚ç ƒ[ƒ‹à—˜‚ğ‹tZ
	double zero_rate_from_df(double df, double t, const Compounding& c);

} // namespace yc
