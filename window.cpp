#include <math.h>
#include <string.h>
#include <stdio.h>
#include "window.hpp"


void Window::configure(Window::Type type, size_t size, float beta)
{
	if(m_type == type && m_size == size && m_beta == beta) {
		return;
	}

	m_type = type;
	m_beta = beta;
	m_data.resize(size);
	m_size = size;

	float sum = 0.0f;

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
			y = 0.42f - 0.5f * cosf(2.0f * M_PI * x) + 0.08f * cosf(4.0f * M_PI * x);
			break;
		case Type::Gauss:
			y = exp(-2.0f * beta * beta * (x - 0.5f) * (x - 0.5f));
			break;
		case Type::Square:
			y = 1.0f;
			break;
		}
		m_data[i] = y;
		sum += y;
	}

	m_gain = (float)size / sum;
}


static const char *k_window_str[] = { 
	"square", "hanning", "hamming", "blackman", "gauss" 
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

