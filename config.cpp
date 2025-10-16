
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


void ConfigWriter::write(const char *key, bool val)
{
	write(key, val ? "true" : "false");
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


ConfigReader::ConfigReader() 
{
	m_root = new Node();
	m_cursor = m_root;
}


void ConfigReader::open(const char *fname) 
{
	FILE *f = fopen(fname, "r");
	if(f == nullptr) {
		fprintf(stderr, "Could not open config file for reading: %s\n", fname);
		return;
	}

	char buf[256];
	while(fgets(buf, sizeof(buf), f)) {
		char *nl = strchr(buf, '\n');
		if(nl) *nl = 0;
		parse_line(buf);
	}
	fclose(f);

	//m_root->dump();
}



void ConfigReader::close()
{
}


void ConfigReader::Node::dump(int depth)
{
	for(auto it : m_attrs) {
		for(int i=0; i<depth; i++) printf("  ");
		printf("- %s = '%s'\n", it.first, it.second);
	}
	for(auto it : kids) {
		for(int i=0; i<depth; i++) printf("  ");
		printf("- %s\n", it.first);
		it.second->dump(depth + 1);
	}
}


ConfigReader::Node *ConfigReader::find(const char *key)
{
	return find(m_root, key);
}


ConfigReader::Node *ConfigReader::find(Node *node, const char *key)
{
	return node->kids[key];
}


void ConfigReader::parse_line(char *line)
{
	char *eq = strchr(line, '=');
	if(eq == nullptr) return;
	*eq = '\0';
	char *key = line;
	const char *val = eq + 1;


	Node * node = m_root;
	for(;;) {
		char *dot = strchr(key, '.');
		if(dot) {
			*dot = 0;
			if(node->kids.find(key) == node->kids.end()) {
				node->kids[strdup(key)] = new Node();
			} else {
			}
			node = node->kids[key];
			key = dot + 1;
		} else {
			break;
		}
	}

	node->m_attrs[strdup(key)] = strdup(val);

}

		
ConfigReader::Node *ConfigReader::Node::find(const char *key)
{
	return kids[key];
}


bool ConfigReader::Node::read(const char *key, bool &val)
{
	if(const char *buf = read_str(key)) {
		val = strcmp(buf, "true") == 0 ? true : false;
		return true;
	} else {
		return false;
	}
}


bool ConfigReader::Node::read(const char *key, bool &val, bool defval)
{
	if(const char *buf = read_str(key)) {
		val = strcmp(buf, "true") == 0 ? true : false;
		return true;
	} else {
		return false;
	}
}

bool ConfigReader::Node::read(const char *key, int &val)
{
	if(const char *buf = read_str(key)) {
		val = atoi(buf);
		return true;
	} else {
		return false;
	}
}


bool ConfigReader::Node::read(const char *key, int &val, int defval)
{
	if(const char *buf = read_str(key)) {
		return atoi(buf);
	} else {
		return defval;
	}
}

bool ConfigReader::Node::read(const char *key, float &val)
{
	char buf[32];

	if(read(key, buf, sizeof(buf))) {
		val = atof(buf);
		return true;
	} else {
		return false;
	}
}


bool ConfigReader::Node::read(const char *key, float &val, float defval)
{
	char buf[32];

	if(read(key, buf, sizeof(buf))) {
		val = atof(buf);
		return true;
	} else {
		val = defval;
		return false;
	}
}


bool ConfigReader::Node::read(const char *key, char *val, size_t maxlen)
{
	const char *c = m_attrs[key];
	if(c) {
		snprintf(val, maxlen, "%s", c);
		return true;
	} else {
		return false;
	}
}


bool ConfigReader::Node::read(const char *key, char *val, size_t maxlen, const char *defval)
{
	const char *c = m_attrs[key];
	if(c) {
		snprintf(val, maxlen, "%s", c);
		val[maxlen - 1] = 0;
		return true;
	} else {
		snprintf(val, maxlen, "%s", defval);
		val[maxlen - 1] = 0;
		return false;
	}
}


const char *ConfigReader::Node::read_str(const char *key)
{
	return  m_attrs[key];
}

