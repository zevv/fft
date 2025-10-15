#pragma once

#include <vector>

class Window {

public:
	enum class Type : int {
		Rectangular, Hamming, Hanning, Blackman, Gauss
	};

	void configure(Type type, size_t size, float sigma = 0.4f);
	const float *data() const { return m_data.data(); }
	size_t size() const { return m_size; }

private:
	Type m_type;
	size_t m_size;
	float m_sigma;
	std::vector<float> m_data;
};

