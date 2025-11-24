
#pragma once

#include <vector>
#include <math.h>

#include "window.hpp"

class Fir {
public:
	enum class Type { HP, LP };

	Fir(size_t size)
		: m_size(size)
	{
		m_buf.resize(size);
		m_kernel.resize(size);
		m_window.configure(Window::Type::Hanning, size);
	}

	~Fir()
	{
	}

	void configure(Type type, Frequency f_cut)
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
	
    double run(double v)
    {
        m_buf[m_buf_head] = v;
        double out = 0.0f;
        size_t k = 0;
        for(int i = (int)m_buf_head; i >= 0; i--) {
            out += m_buf[i] * m_kernel[k];
            k++;
        }
        for(int i = (int)m_size - 1; i > (int)m_buf_head; i--) {
            out += m_buf[i] * m_kernel[k];
            k++;
        }
        m_buf_head = (m_buf_head + 1) % m_size;
        return out;
    }

private:
	size_t m_size{};
	std::vector<float> m_kernel{};
	std::vector<double> m_buf{};
	size_t m_buf_head{};
	Window m_window{};
};

