# Makefile

# Define the compiler
GLSLC = glslc

# Define the source file extensions
VERT_EXT = .vert
FRAG_EXT = .frag

# Define the output file extension
SPV_EXT = .spv

# Find all the source files
VERT_SRC = $(wildcard *Shader$(VERT_EXT))
FRAG_SRC = $(wildcard *Shader$(FRAG_EXT))

# Define the object files
VERT_OBJ = $(patsubst %Shader$(VERT_EXT),%Vert$(SPV_EXT),$(VERT_SRC))
FRAG_OBJ = $(patsubst %Shader$(FRAG_EXT),%Frag$(SPV_EXT),$(FRAG_SRC))

# The default rule - compiling our main program:
all: $(VERT_OBJ) $(FRAG_OBJ)

# Rule for the vertex shader
%Vert$(SPV_EXT): %Shader$(VERT_EXT)
	$(GLSLC) $< -o $@

# Rule for the fragment shader
%Frag$(SPV_EXT): %Shader$(FRAG_EXT)
	$(GLSLC) $< -o $@


# Rule for the vertex shader
Vert$(SPV_EXT): Shader$(VERT_EXT)
	$(GLSLC) $< -o $@

# Rule for the fragment shader
Frag$(SPV_EXT): Shader$(FRAG_EXT)
	$(GLSLC) $< -o $@


# Clean rule
clean:
	rm -f *Vert$(SPV_EXT) *Frag$(SPV_EXT)