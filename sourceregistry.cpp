
#include <vector>

#include "sourceregistry.hpp"


static std::vector<SourceInfo> &registry()
{
	static std::vector<SourceInfo> instance;
	return instance;
}


SourceRegistry::SourceRegistry(SourceInfo info)
{
	registry().push_back(info);
}


Source *SourceRegistry::create(const char *name, SDL_AudioSpec &dst_spec, char *args)
{
	for(auto &reg : registry()) {
		if(strcmp(reg.name, name) == 0) {
			Source *reader = reg.fn_create(dst_spec, args);
			reader->dump(stdout);
			assert(reader != nullptr);
			return reader;
		}
	}
	return nullptr;
}


