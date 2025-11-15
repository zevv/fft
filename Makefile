
BIN = fft

IMGUI_DIR = /home/ico/external/imgui

SRC += main.cpp
SRC += app.cpp
SRC += window.cpp
SRC += panel.cpp
SRC += channelmap.cpp
SRC += widget.cpp
SRC += widgetregistry.cpp
SRC += widget-wave.cpp
SRC += widget-spectrum.cpp
SRC += widget-waterfall.cpp
SRC += widget-waterfall2.cpp
SRC += widget-histogram.cpp
SRC += widget-xy.cpp
SRC += widget-channels.cpp
SRC += stream.cpp
SRC += wavecache.cpp
SRC += misc.cpp
SRC += source.cpp
SRC += sourceregistry.cpp
SRC += player.cpp
SRC += capture.cpp
SRC += source-audio.cpp
SRC += source-jack.cpp
SRC += source-file.cpp
SRC += source-generator.cpp
SRC += config.cpp
SRC += biquad.cpp
SRC += rb.cpp
SRC += fft.cpp
SRC += histogram.cpp
SRC += misc.cpp
SRC += $(IMGUI_DIR)/imgui.cpp 
SRC += $(IMGUI_DIR)/imgui_demo.cpp
SRC += $(IMGUI_DIR)/imgui_draw.cpp 
SRC += $(IMGUI_DIR)/imgui_tables.cpp
SRC += $(IMGUI_DIR)/imgui_widgets.cpp
SRC += $(IMGUI_DIR)/backends/imgui_impl_sdl3.cpp
SRC += $(IMGUI_DIR)/backends/imgui_impl_sdlrenderer3.cpp

PKG += fftw3f sdl3 jack


PKG_CFLAGS := $(shell pkg-config $(PKG) --cflags)
PKG_LIBS := $(shell pkg-config $(PKG) --libs)

OBJS = $(SRC:.cpp=.o)
DEPS = $(OBJS:.o=.d)

CXXFLAGS += -std=c++23
CXXFLAGS += -g 
CXXFLAGS += -Wall -Wformat -Werror
CXXFLAGS += -Wno-unused-but-set-variable -Wno-unused-variable -Wno-format-truncation
CXXFLAGS += -O3 -ffast-math
CXXFLAGS += -march=native
CXXFLAGS += -MMD
CXXFLAGS += -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += $(PKG_CFLAGS)

LIBS += -ldl $(PKG_LIBS)

ifdef bitline
CXXFLAGS += -DBITLINE
endif

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

CCACHE := $(shell which ccache)
ifneq ($(CCACHE),)
CXX := $(CCACHE) $(CXX)
endif

CFLAGS = $(CXXFLAGS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

perf: 
	perf record -F 99 -g -- ./$(BIN)

flamegraph: perf.data
	perf script \
		| /home/ico/external/FlameGraph/stackcollapse-perf.pl \
		| /home/ico/external/FlameGraph/flamegraph.pl > flamegraph.svg

clean:
	rm -f $(BIN) $(OBJS) $(DEPS)
	rm -f perf.data* flamegraph.svg

-include $(DEPS)
