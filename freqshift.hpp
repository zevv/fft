
#pragma once

#include <math.h>
#include <complex.h>

class ComplexOscillator {
private:
	Time m_phase{0.0};
	Time m_dphase{0.0};

public:
	void set_frequency(Frequency freq)
	{
		m_dphase = freq;
	}

	std::complex<double> run()
	{
		double I = sin(2.0 * M_PI * m_phase);
		double Q = cos(2.0 * M_PI * m_phase);
		std::complex<double> out(I, Q);
		m_phase = fmod(m_phase + m_dphase, 1.0);
		return out;
	}
};




class FreqShift {

private:
	Hilbert m_hilbert_in;
	Hilbert m_hilbert_out[2];

	ComplexOscillator m_osc_shift{};

public:

	void set_frequency(Frequency freq)
	{
		m_osc_shift.set_frequency(freq);
	}

	double run(double v)
	{
		auto v_analytical = m_hilbert_in.run(v);
		auto osc = m_osc_shift.run();
		auto v_shifted = v_analytical * osc;
		auto v_shifted_i = m_hilbert_out[0].run(v_shifted.real()).real();
		auto v_shifted_q = m_hilbert_out[1].run(v_shifted.imag()).imag();
		return 0.5 * (v_shifted_i - v_shifted_q);
	}

};
