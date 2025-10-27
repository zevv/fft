
#include <stdio.h>
#include <algorithm>

#include "histogram.hpp"

Histogram::Histogram()
	: m_min(0.0)
	, m_binwidth(1.0)
{
	m_bin.resize(64, 0);
}


Histogram::Histogram(int bins, double min, double max)
	: m_min(min)
	, m_binwidth((max - min) / bins)
	
{
	m_bin.resize(bins, 0);
}


void Histogram::clear()
{
	std::fill(m_bin.begin(), m_bin.end(), 0);
	m_total = 0;
}


void Histogram::set_nbins(size_t n)
{
	m_bin.resize(n, 0);
}


void Histogram::set_range(double min, double max)
{
	m_min = min;
	m_binwidth = (max - min) / m_bin.size();
}


size_t Histogram::get_peak() const
{
	size_t peak = m_bin[0];
	for(size_t i=1; i<m_bin.size(); i++) {
		peak = std::max(peak, m_bin[i]);
	}
	return peak;
}


double Histogram::get_percentile(double p) const
{
	size_t thresh = m_total * p;
	size_t sum = 0;
	for(size_t i = 0, sum = 0; i < m_bin.size(); i++) {
		sum += m_bin[i];
		if(sum >= thresh) {
			return m_min + m_binwidth * (i + 0.5);
		}
	}
	return m_min + m_binwidth * (m_bin.size() - 0.5);
}
