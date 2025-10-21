#include <math.h>
#include <string.h>
#include <stdio.h>
#include "window.hpp"

float kaiser(float x, float beta)
{
	float I0_beta = 1.0f;
	float factorial = 1.0f;
	float beta_over_2 = beta / 2.0f;
	float x_pow_2n = 1.0f;
	const int N = 25;
	for(int n=1; n<=N; n++) {
		factorial *= (float)n;
		x_pow_2n *= (beta_over_2 * beta_over_2);
		I0_beta += x_pow_2n / (factorial * factorial);
	}

	float arg = beta * sqrtf(1.0f - 4.0f * (x - 0.5f) * (x - 0.5f));
	float I0_arg = 1.0f;
	factorial = 1.0f;
	x_pow_2n = 1.0f;
	for(int n=1; n<=N; n++) {
		factorial *= (float)n;
		x_pow_2n *= (arg * arg);
		I0_arg += x_pow_2n / (factorial * factorial);
	}

	return I0_arg / I0_beta;
}


void Window::configure(Window::Type type, size_t size, float beta)
{
	if(m_type == type && m_data.size() == size && m_beta == beta) {
		return;
	}

	m_type = type;
	m_beta = beta;
	m_data.resize(size);

	Sample sum = 0.0f;

	for(size_t i=0; i<size; i++) {
		Time x = (float)i / (float)(size - 1);
		Sample y;

		switch(type) {
		case Type::Square:
			y = 1.0f;
			break;
		case Type::Hamming:
			y = 0.54f - 0.46f * cosf(2.0f * M_PI * x);
			break;
		case Type::Hanning:
			y = 0.5f * (1.0f - cosf(2.0f * M_PI * x));
			break;
		case Type::Blackman:
			y = 0.42f - 0.5f * cosf(2.0f * M_PI * x) + 0.08f * cosf(4.0f * M_PI * x);
			break;
		case Type::Gauss:
			y = exp(-2.0f * beta * beta * (x - 0.5f) * (x - 0.5f));
			break;
		case Type::Kaiser:
			y = kaiser(x, beta);
			break;
		}
		m_data[i] = y;
		sum += y;
	}

	Sample m_gain = (float)size / sum;

	for(size_t i=0; i<size; i++) {
		m_data[i] *= m_gain;
	}
}


void Window::set_size(size_t size)
{
	configure(m_type, size, m_beta);
}


static const char *k_window_str[] = { 
	"square", "hanning", "hamming", "blackman", "gauss", "kaiser"
};


const char **Window::type_names()
{
	return k_window_str;
}


size_t Window::type_count()
{
	return sizeof(k_window_str) / sizeof(k_window_str[0]);
}


const char *Window::type_to_str(Type t)
{
	size_t count = type_count();
	const char **names = type_names();
	for(size_t i=0; i<count; i++) {
		if((int)t == (int)i) return names[i];
	}
	return "square";
}


Window::Type Window::str_to_type(const char *s)
{
	size_t count = type_count();
	const char **names = type_names();
	for(size_t i=0; i<count; i++) {
		if(strcmp(s, names[i]) == 0) return (Type)i;
	}
	return Type::Square;
}

