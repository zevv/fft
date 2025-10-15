
#pragma once

#include <algorithm>

class View {

public:
	View()
		: count(1024)
		, offset(0)
		, m_count_min(100)
		, m_count_max(1024 * 1024)
		, m_offset_min(0)
		, m_offset_max(1024 * 1024)
	{}

	void zoom(float factor) {
		count = count * factor;
		clamp();
	};

	void pan(int delta) {
		offset += delta;
		clamp();
	};

	void clamp() {
		float a1 = 0.9;
		float a2 = 1 - a1;
		if(count > m_count_max) count = count * a1 + m_count_max * a2;
		if(count < m_count_min) count = count * a1 + m_count_min * a2;
		if(offset < m_offset_min) offset = offset * a1 + m_offset_min * a2;
		if(offset > m_offset_max - count) offset = offset * a1 + (m_offset_max - count) * a2;
	};
	
	int count;
	int offset;

private:

	int m_count_min;
	int m_count_max;
	int m_offset_min;
	int m_offset_max;
};
