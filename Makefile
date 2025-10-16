
EXE = fft

IMGUI_DIR = /home/ico/external/imgui

SRC += main.cpp
SRC += window.cpp
SRC += panel.cpp
SRC += widget.cpp
SRC += stream.cpp
SRC += config.cpp
SRC += $(IMGUI_DIR)/imgui.cpp 
SRC += $(IMGUI_DIR)/imgui_demo.cpp
SRC += $(IMGUI_DIR)/imgui_draw.cpp 
SRC += $(IMGUI_DIR)/imgui_tables.cpp
SRC += $(IMGUI_DIR)/imgui_widgets.cpp
SRC += $(IMGUI_DIR)/backends/imgui_impl_sdl3.cpp $(IMGUI_DIR)/backends/imgui_impl_sdlrenderer3.cpp

OBJS = $(SRC:.cpp=.o)
DEPS = $(OBJS:.o=.d)

CXXFLAGS += -std=c++17 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -g -Wall -Wformat -Werror
CXXFLAGS += -O3
CXXFLAGS += -MMD

CXXFLAGS += `pkg-config sdl3 fftw3 fftw3f --cflags`
LIBS += -ldl `pkg-config sdl3 fftw3 fftw3f --libs`

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
