
#pragma once

#include <vector>
#include <math.h>

#include "window.hpp"

class Fir {
public:
	enum class Type { HP, LP };

	Fir(size_t size);
	void configure(Type type, Frequency f_cut);
	
    double run(double v)
    {
        m_buf[m_buf_head] = v;
        double out = 0.0f;
        size_t k = 0;
        for(int i = (int)m_buf_head; i >= 0; i--) {
            out += m_buf[i] * m_kernel[k++];
        }
        for(int i = (int)m_size - 1; i > (int)m_buf_head; i--) {
            out += m_buf[i] * m_kernel[k++];
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

