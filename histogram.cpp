
#include <stdio.h>
#include <algorithm>

#include "histogram.hpp"

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


void Histogram::normalize()
{
	size_t v_min = m_bin[0];
	size_t v_max = m_bin[0];
	for(size_t i=1; i<m_bin.size(); i++) {
		v_min = std::min(v_min, m_bin[i]);
		v_max = std::max(v_max, m_bin[i]);
	}
	double range = v_max - v_min;
	if(range > 0) {
		for(size_t i=0; i<m_bin.size(); i++) {
			m_bin[i] = (m_bin[i] - v_min) / range;
		}
	} else {
		for(size_t i=0; i<m_bin.size(); i++) {
			m_bin[i] = 0;
		}
	}
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
