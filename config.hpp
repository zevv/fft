#pragma once

#include <stdio.h>
#include <vector>


struct ConfigWriter {
public:

	ConfigWriter();

	void open(const char *fname);
	void close();

	void push(const char *key);
	void push(int key);
	void pop();

	void write(const char *key, int val);
	void write(const char *key, float val);
	void write(const char *key, const char *val);
	
private:
	FILE *m_f;
	std::vector<char *> m_prefixes;
};

