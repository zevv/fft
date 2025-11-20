
#pragma once

#include <vector>


template<typename T>
class Histogram {
public:
	Histogram() = default;

	Histogram(size_t bins, T min, T max)
		: m_bins(bins), m_min(min), m_max(max)
	{
		m_data.resize(bins);
	}

	void add(T value)
	{
		size_t bin = ((double)(value - m_min) / (double)(m_max - m_min) * m_bins);
		if(bin >= 0 && bin < m_data.size()) {
			m_data[bin]++;
		}
	}

	void set_range(T min, T max)
	{
		m_min = min;
		m_max = max;
	}

	void set_nbins(size_t bins)
	{
		m_bins = bins;
		m_data.resize(bins);
	}

	void clear()
	{
		std::fill(m_data.begin(), m_data.end(), 0);
	}

	std::vector<size_t> &bins()
	{
		return m_data;
	}

	T find_percentile(double p) const
	{
		size_t total = 0;
		for(auto &b : m_data) total += b;
		size_t target = total * p;
		size_t sum = 0;
		for(size_t i=0; i<m_bins; i++) {
			sum += m_data[i];
			if(sum >= target) {
				T val = m_min + (T)((double)i / (double)m_bins * (double)(m_max - m_min));
				return val;
			}
		}
		return m_max;
	}

	T peak() const
	{
		size_t peak = 0;
		for(auto &b : m_data) {
			if(b > peak) peak = b;
		}
		return peak;
	}

	void add(const Histogram<T> &other)
	{
		for(size_t i=0; i<m_bins; i++) {
			m_data[i] += other.m_data[i];
		}
	}

private:
	size_t m_bins;
	T m_min;
	T m_max;
	std::vector<size_t> m_data;
};

