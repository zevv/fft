
#pragma once

#include <vector>

class Histogram {
public:
	Histogram();
	Histogram(int bins, double min, double max);

	void clear();
	void set_nbins(size_t n);
	void set_range(double min, double max);
	std::vector<size_t> &bins() { return m_bin; }

	inline void add(double v) {
		int bin = (v - m_min) / m_binwidth;
		if (bin >= 0 && bin < (int)m_bin.size()) {
			m_bin[bin]++;
			m_total++;
		}
	}

	double get_percentile(double p) const;
	size_t get_peak() const;

private:
	std::vector<size_t> m_bin{};
	size_t m_total{};
	double m_min{};
	double m_binwidth{};

};
