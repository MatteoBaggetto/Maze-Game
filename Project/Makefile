CFLAGS = -std=c++17 -O2
INC = -I./headers
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXrandr

compile: project.cpp
	g++ $(CFLAGS) $(INC) -o project.run project.cpp $(LDFLAGS)

run: 
	./project.run

clean:
	rm -f project.run

shader:
	cd shaders && make

all: shader compile run

shader_run: shader run

.PHONY: clean all