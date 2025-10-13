#include <math.h>
#include "window.hpp"

void Window::configure(Window::Type type, size_t size, float alpha)
{
	if(m_type == type && m_size == size && m_alpha == alpha) {
		return;
	}

	m_type = type;
	m_alpha = alpha;
	m_data.resize(size);
	m_size = size;

	for(size_t i=0; i<size; i++) {
		float x = (float)i / (float)(size - 1);
		float y = 1.0f;

		switch(type) {
		case Type::Hamming:
			y = 0.54f - 0.46f * cosf(2.0f * M_PI * x);
			break;
		case Type::Hanning:
			y = 0.5f * (1.0f - cosf(2.0f * M_PI * x));
			break;
		case Type::Blackman:
			y = (1.0f - alpha) / 2.0f - 0.5f * cosf(2.0f * M_PI * x) + alpha / 2.0f * cosf(4.0f * M_PI * x);
			break;
		case Type::Gauss:
			y = expf(-0.5f * powf((x - 0.5f) / (alpha * 0.5f), 2.0f));
			break;
		case Type::Rectangular:
			y = 1.0f;
			break;
		}
		m_data[i] = y;
	}
}


