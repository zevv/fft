#pragma once

#include <vector>
#include "types.hpp"

class Window {

public:
	enum class Type : int {
		Square, Hamming, Hanning, Blackman, Gauss, Kaiser
	};

	static const char **type_names();
	static size_t type_count();
	static const char *type_to_str(Type t);
	static Type str_to_type(const char *s);

	void configure(Type type, size_t size, float beta = 6.0f);
	void set_size(size_t size);
	size_t size() const { return m_data.size(); };
	Sample gain() const { return m_gain; };
	std::vector<Sample> &data() { return m_data; };

private:
	Type m_type;
	float m_beta;
	Sample m_gain;
	std::vector<Sample> m_data;
};
