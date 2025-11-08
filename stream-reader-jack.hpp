
#include <SDL3/SDL_audio.h>
#include <jack/jack.h>

#include "stream-reader.hpp"


class StreamReaderJack : public StreamReader {
public:
	StreamReaderJack(SDL_AudioSpec &dst_spec);
	~StreamReaderJack();

	void open() override;
	void poll() override;

	int process_callback(jack_nframes_t nframes);

private:
	SDL_AudioSpec m_src_spec{};
	jack_client_t *m_jack_client{};
	
	struct Port {
		char name[32];
		jack_port_t *jack_port;
	};

	size_t m_jack_buffer_size{};
	std::vector<Port> m_ports;
	std::vector<float> m_buffer;
};

