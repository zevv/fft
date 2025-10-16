#pragma once

#include <vector>

class Window {

public:
	enum class Type : int {
		Rectangular, Hamming, Hanning, Blackman, Gauss
	};

	void configure(Type type, size_t size, float beta = 6.0f);
	const float *data() const { return m_data.data(); }
	size_t size() const { return m_size; }
	float gain() const { return m_gain; }

private:
	Type m_type;
	size_t m_size;
	float m_beta;
	float m_gain;
	std::vector<float> m_data;
};

