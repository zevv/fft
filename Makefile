
EXE = fft

IMGUI_DIR = /home/ico/external/imgui

SRC += main.cpp
SRC += window.cpp
SRC += panel.cpp
SRC += channelmap.cpp
SRC += widget.cpp
SRC += widget-wave.cpp
SRC += widget-spectrum.cpp
SRC += widget-waterfall.cpp
SRC += widget-histogram.cpp
SRC += stream.cpp
SRC += config.cpp
SRC += rb.cpp
SRC += fft.cpp
SRC += histogram.cpp
SRC += $(IMGUI_DIR)/imgui.cpp 
SRC += $(IMGUI_DIR)/imgui_demo.cpp
SRC += $(IMGUI_DIR)/imgui_draw.cpp 
SRC += $(IMGUI_DIR)/imgui_tables.cpp
SRC += $(IMGUI_DIR)/imgui_widgets.cpp
SRC += $(IMGUI_DIR)/backends/imgui_impl_sdl3.cpp
SRC += $(IMGUI_DIR)/backends/imgui_impl_sdlrenderer3.cpp

OBJS = $(SRC:.cpp=.o)
DEPS = $(OBJS:.o=.d)

CXXFLAGS += -std=c++23 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -g 
CXXFLAGS += -Wall -Wformat -Werror
CXXFLAGS += -Wno-unused-but-set-variable -Wno-unused-variable
CXXFLAGS += -O3 -ffast-math
CXXFLAGS += -march=native
CXXFLAGS += -MMD

CXXFLAGS += `pkg-config sdl3 fftw3 fftw3f --cflags`
LIBS += -ldl `pkg-config sdl3 fftw3 fftw3f --libs`

ifdef clang
CXX=clang++
LD=clang++
endif

ifdef asan
CXXFLAGS += -fsanitize=address 
LDFLAGS += -fsanitize=address 
endif

ifdef smem
CXX=clang++-19
LD=clang++-19
CXXFLAGS += -fsanitize=memory -fsanitize-memory-track-origins=2
LDFLAGS += -fsanitize=memory -fsanitize-memory-track-origins=2
endif

CFLAGS = $(CXXFLAGS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS) $(DEPS)

-include $(DEPS)
