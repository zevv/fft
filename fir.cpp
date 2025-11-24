
#include "fir.hpp"


Fir::Fir(size_t size)
	: m_size(size)
{
	m_buf.resize(size);
	m_kernel.resize(size);
	m_window.configure(Window::Type::Hanning, size);
}


void Fir::configure(Fir::Type type, Frequency f_cut)
{
	// sin(x)/x kernel
	double center = (double)(m_size - 1) / 2.0;
	for(size_t i=0; i<m_size; i++) {
		double n = (double)i - center;
		if(fabs(n) < 1e-9) {
			m_kernel[i] = 2.0 * f_cut;
		} else {
			m_kernel[i] = sin(2.0 * M_PI * f_cut * n) / (M_PI * n);
		}
	}

	// windowing
	auto wdata = m_window.data();
	for(size_t i=0; i<m_size; i++) {
		m_kernel[i] *= wdata[i];
	}

	// high-pass conversion
	if(type == Type::HP) {
		for(size_t i=0; i<m_size; i++) {
			m_kernel[i] = -m_kernel[i];
		}
		if(m_size % 2 != 0) {
			m_kernel[(m_size - 1)/ 2] += 1.0;
		}
	}
}
