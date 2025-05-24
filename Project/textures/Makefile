# Makefile - needs ImageMagick installed (sudo apt-get install imagemagick)

PNG_SOURCES := $(filter-out %-small.png, $(wildcard *.png))
JPG_SOURCES := $(filter-out %-small.jpg, $(wildcard *.jpg))
PNG_TARGETS := $(patsubst %.png,%-small.png,$(PNG_SOURCES))
JPG_TARGETS := $(patsubst %.jpg,%-small.jpg,$(JPG_SOURCES))

all: $(PNG_TARGETS) $(JPG_TARGETS)

%-small.png: %.png
	convert -resize 20% $< $@

%-small.jpg: %.jpg
	convert -resize 20% $< $@