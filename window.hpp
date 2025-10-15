#pragma once

#include <vector>

class Window {

public:
	enum class Type : int {
		Rectangular, Hamming, Hanning, Blackman, Gauss
	};

	void configure(Type type, size_t size, float sigma = 0.4f);
	double get_data(size_t index) { return m_data[index]; }

private:
	Type m_type;
	size_t m_size;
	float m_sigma;
	std::vector<double> m_data;
};

