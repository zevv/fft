
#include <vector>

#include "sourceregistry.hpp"


std::vector<Source::Info> &SourceRegistry::registry()
{
	static std::vector<Source::Info> instance;
	return instance;
}


SourceRegistry::SourceRegistry(Source::Info info)
{
	registry().push_back(info);
}


Source *SourceRegistry::create(const char *name, SDL_AudioSpec &dst_spec, char *args)
{
	for(auto &reg : registry()) {
		if(strcmp(reg.name, name) == 0) {
			Source *source = reg.fn_new(dst_spec, args);
			source->dump(stdout);
			assert(source != nullptr);
			return source;
		}
	}
	return nullptr;
}
