#pragma once

#include <vector>
#include "types.hpp"

class Window {

public:
	enum class Type : int {
		Square, Hamming, Hanning, Blackman, Gauss, Kaiser
	};

	Window();
	Window(Type type, size_t size, float beta);
	static const char **type_names();
	static size_t type_count();
	static const char *type_to_str(Type t);
	static Type str_to_type(const char *s);

	void configure(Type type, size_t size, float beta = 6.0f);

	size_t size() const { return m_data.size(); };
	double beta() const { return m_beta; };
	double gain() const { return m_gain; };
	Type type() const { return m_type; };

	std::vector<float> &data() { return m_data; };

private:
	Type m_type;
	double m_beta;
	double m_gain;
	std::vector<float> m_data;
};
