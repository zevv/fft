#pragma once

#include <vector>

class Window {

public:
	enum class Type : int {
		Hamming, Hanning, Blackman, Gauss, Rectangular,
	};

	void configure(Type type, size_t size, float alpha = 0.4f);
	double get_data(size_t index) { return m_data[index]; }

private:
	Type m_type;
	size_t m_size;
	float m_alpha;
	std::vector<double> m_data;
};

