
/*
 * Implementation of simple second order IIR biquad filters: low pass, high
 * pass, band pass and band stop.
 *
 * http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
 */

#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <algorithm>

#include "biquad.hpp"

Biquad::Biquad()
{
	m_first = true;
	configure(Type::LP, 1000, 0.707);
}


void Biquad::configure(Type type, float freq, float Q)
{
	freq = std::clamp(freq, 0.0f, 0.5f);
	float a0 = 0.0, a1 = 0.0, a2 = 0.0;
	float b0 = 0.0, b1 = 0.0, b2 = 0.0;

	float w0 = M_PI * freq * 0.5;
	float alpha = sin(w0) / (2.0*Q);
	float cos_w0 = cos(w0);

	switch(type) {

		case Type::LP:
			b0 = (1.0 - cos_w0) / 2.0;
			b1 = 1.0 - cos_w0;
			b2 = (1.0 - cos_w0) / 2.0;
			a0 = 1.0 + alpha;
			a1 = -2.0 * cos_w0;
			a2 = 1.0 - alpha;
			break;

		case Type::HP:
			b0 = (1.0 + cos_w0) / 2.0;
			b1 = -(1.0 + cos_w0);
			b2 = (1.0 + cos_w0) / 2.0;
			a0 = 1.0 + alpha;
			a1 = -2.0 * cos_w0;
			a2 = 1.0 - alpha;
			break;

		case Type::BP:
			b0 = Q * alpha;
			b1 = 0.0;
			b2 = -Q * alpha;
			a0 = 1.0 + alpha;
			a1 = -2.0 * cos_w0;
			a2 = 1.0 - alpha;
			break;

		case Type::BS:
			b0 = 1.0;
			b1 = -2.0 * cos_w0;
			b2 = 1.0;
			a0 = a2 = 1.0 + alpha;
			a1 = -2.0 * cos_w0;
			a2 = 1.0 - alpha;
			break;

	}

	if(a0 == 0.0) return;
	m_b0_a0 = b0 / a0;
	m_b1_a0 = b1 / a0;
	m_b2_a0 = b2 / a0;
	m_a1_a0 = a1 / a0;
	m_a2_a0 = a2 / a0;
	m_initialized = true;

}

/*
 * End
 */
