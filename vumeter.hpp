
#pragma once

#include <imgui.h>


class VuMeter {

public:

	void update(Sample *buf, size_t nframes, size_t stride);
	void draw(void);

private:

	double m_peak{};
	ImVec4 m_col{};
};

