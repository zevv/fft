
#include <SDL3/SDL_audio.h>
#include <jack/jack.h>

#include "source.hpp"

class SourceJack : public Source {
public:
	SourceJack(SourceInfo &info, SDL_AudioSpec &dst_spec, char *args);
	~SourceJack();

	void open() override;
	void pause() override;
	void resume() override;

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


SourceJack::SourceJack(SourceInfo &info, SDL_AudioSpec &dst_spec, char *args)
	: Source(info, dst_spec)
{
}


SourceJack::~SourceJack()
{
	jack_deactivate(m_jack_client);
	for(auto &port : m_ports) {
		jack_port_unregister(m_jack_client, port.jack_port);
	}
	jack_client_close(m_jack_client);
}


static int process_callback_(jack_nframes_t nframes, void *arg)
{
	SourceJack *reader = static_cast<SourceJack *>(arg);
	return reader->process_callback(nframes);
}


int SourceJack::process_callback(jack_nframes_t nframes)
{
	size_t stride = m_ports.size();
	for(size_t i=0; i<m_ports.size(); i++) {
		auto &port = m_ports[i];
		float *buf = m_buffer.data() + i;
		float *p = (float *)jack_port_get_buffer(port.jack_port, nframes);
		for(size_t j=0; j<nframes; j++) {
			buf[j * stride] = p[j];
		}
	}
	SDL_PutAudioStreamData(m_sdl_stream, m_buffer.data(), nframes * stride * sizeof(float));
	return 0;
}


void SourceJack::open()
{
	jack_status_t status;
	m_jack_client = jack_client_open("fft", JackNullOption, &status, NULL);
	if(m_jack_client == nullptr) {
		fprintf(stderr, "SourceJack jack_client_open failed: %x\n", status);
		return;
	}

	for(int i=0; i<m_dst_spec.channels; i++) {
		Port port;
		snprintf(port.name, sizeof(port.name), "input_%d", i+1);
		port.jack_port = jack_port_register(m_jack_client, port.name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
		if(port.jack_port == nullptr) {
			fprintf(stderr, "SourceJack jack_port_register failed for port %d\n", i);
			return;
		}
		m_ports.push_back(port);
	}
	
	m_src_spec.freq = jack_get_sample_rate(m_jack_client);
	m_src_spec.format = SDL_AUDIO_F32;
	m_src_spec.channels = m_dst_spec.channels;

	m_jack_buffer_size = jack_get_buffer_size(m_jack_client);
	m_buffer.resize(m_jack_buffer_size * m_src_spec.channels);

	m_sdl_stream = SDL_CreateAudioStream(&m_src_spec, &m_dst_spec);
	if(m_sdl_stream == nullptr) {
		fprintf(stderr, "SourceFile SDL_CreateAudioStream failed: %s\n", SDL_GetError());
		return;
	}

	jack_set_process_callback(m_jack_client, process_callback_, this);
	jack_activate(m_jack_client);
}


void SourceJack::pause()
{
	jack_deactivate(m_jack_client);
}


void SourceJack::resume()
{
	jack_activate(m_jack_client);
}


REGISTER_STREAM_READER(SourceJack,
	.name = "jack",
	.description = "Jack audio source",
);

