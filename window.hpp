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
	const Sample *data() const { return m_data.data(); }
	size_t size() const { return m_size; }

private:
	Type m_type;
	size_t m_size;
	float m_beta;
	std::vector<Sample> m_data;
};
