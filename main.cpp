
#include <assert.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <fftw3.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "app.hpp"


int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	// scope to ensure App destructor called before SDL_Quit
	{
	App app = App(nullptr, nullptr);
	app.init(argc, argv);
	app.run();
	app.exit();
	}

	fftwf_cleanup();
	SDL_Quit();

	return 0;
}

