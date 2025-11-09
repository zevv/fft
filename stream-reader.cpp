
#include <vector>
#include "stream-reader.hpp"

static std::vector<StreamReaderInfo> &registry()
{
	static std::vector<StreamReaderInfo> instance;
	return instance;
}


StreamReaderReg::StreamReaderReg(StreamReaderInfo info)
{
	registry().push_back(info);
}

StreamReader *StreamReaderReg::create(const char *name, SDL_AudioSpec &dst_spec, char *args)
{
	for(auto &reg : registry()) {
		if(strcmp(reg.name, name) == 0) {
			StreamReader *reader = reg.fn_create(dst_spec, args);
			reader->dump(stdout);
			assert(reader != nullptr);
			return reader;
		}
	}
	return nullptr;
}


