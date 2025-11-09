
#include <vector>

#include "sourceregistry.hpp"


std::vector<SourceInfo> &SourceRegistry::get_registry()
{
	static std::vector<SourceInfo> instance;
	return instance;
}


SourceRegistry::SourceRegistry(SourceInfo info)
{
	get_registry().push_back(info);
}


Source *SourceRegistry::create(const char *name, SDL_AudioSpec &dst_spec, char *args)
{
	for(auto &reg : get_registry()) {
		if(strcmp(reg.name, name) == 0) {
			Source *source = reg.fn_create(dst_spec, args);
			source->dump(stdout);
			assert(source != nullptr);
			return source;
		}
	}
	return nullptr;
}


