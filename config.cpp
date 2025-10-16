
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "config.hpp"

ConfigWriter::ConfigWriter() 
	: m_f(nullptr)
	, m_prefixes()
{
	
}


void ConfigWriter::open(const char *fname) 
{
	m_f = fopen(fname, "w");
	if(m_f == nullptr) {
		fprintf(stderr, "Could not open config file for writing: %s\n", fname);
		return;
	}
}


void ConfigWriter::close() 
{
	if(m_f) {
		fclose(m_f);
		m_f = nullptr;
	}
}


void ConfigWriter::push(int key)
{
	char buf[16];
	snprintf(buf, sizeof(buf), "%d", key);
	push(buf);
}

	
void ConfigWriter::push(const char *key)
{
	m_prefixes.push_back(strdup(key));
}


void ConfigWriter::pop()
{
	if(!m_prefixes.empty()) {
		free(m_prefixes.back());
		m_prefixes.pop_back();
	}
}


void ConfigWriter::write(const char *key, int val)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "%d", val);
	write(key, buf);
}


void ConfigWriter::write(const char *key, float val)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "%.7g", val);
	write(key, buf);
}


void ConfigWriter::write(const char *key, const char *val)
{
	if(m_f) {
		for(auto p : m_prefixes) {
			fprintf(m_f, "%s.", p);
		}
		fprintf(m_f, "%s=%s\n", key, val);
	}
}
