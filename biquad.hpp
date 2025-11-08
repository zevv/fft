#ifndef lib_biquad_h
#define lib_biquad_h

#include "types.hpp"

class Biquad {
public:

	enum class Type { LP, HP, BP, BS };
	Biquad();
	void configure(Type type, float freq, float Q = 0.7071);

	float run(float v_in)
	{
		float x0 = v_in;
		float y0;

		if(m_first) {
			m_y1 = m_y2 = m_x1 = m_x2 = x0;
			m_first = false;
		}

		y0 = m_b0_a0 * x0 +
			m_b1_a0 * m_x1 +
			m_b2_a0 * m_x2 -
			m_a1_a0 * m_y1 -
			m_a2_a0 * m_y2;

		m_x2 = m_x1;
		m_x1 = x0;

		m_y2 = m_y1;
		m_y1 = y0;

		return y0;
	}

private:
	float m_x1{}, m_x2{};
	float m_y1{}, m_y2{};
	float m_b0_a0{}, m_b1_a0{}, m_b2_a0{};
	float m_a1_a0{}, m_a2_a0{};
	bool m_initialized{false};
	bool m_first{true};
};

#endif
