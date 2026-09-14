# Desk Buddy - helper targets.
#
#   make preview   render the face on this machine into preview/*.png
#   make gif       render the README animation into preview/deskbuddy.gif
#   make build     compile the firmware   (pio run)
#   make flash     compile + upload       (pio run -t upload)
#   make monitor   open the serial console
#   make clean

HOST_SRC = tools/host/preview.cpp src/Face.cpp src/Effects.cpp src/Shapes.cpp \
           src/Personality.cpp
HOST_FLAGS = -std=c++14 -O2 -Wall -DDESKBUDDY_HOST=1 -Isrc -Itools/host
EMOTIONS = neutral happy excited sad angry surprised sleepy love curious \
           suspicious dizzy bored

.PHONY: preview gif build flash monitor clean

preview/preview: $(HOST_SRC) tools/host/HostGfx.h src/Face.h
	@mkdir -p preview
	$(CXX) $(HOST_FLAGS) -o $@ $(HOST_SRC)

preview: preview/preview
	./preview/preview sheet preview/sheet.bin
	python3 tools/host/frames_to_png.py preview/sheet.bin preview/emotions.png \
		--cols 4 --scale 3 --labels neutral,happy,excited,sad,angry,surprised,sleepy,love,curious,suspicious,dizzy,bored
	./preview/preview anim 8 preview/anim.bin
	python3 tools/host/frames_to_png.py preview/anim.bin preview/timeline.png \
		--cols 12 --scale 1
	@echo "open preview/emotions.png"

gif: preview/preview
	python3 tools/host/make_gif.py preview/deskbuddy.gif

build:
	pio run

flash:
	pio run -t upload

monitor:
	pio device monitor

clean:
	rm -rf .pio preview/preview preview/*.bin preview/timeline.png
