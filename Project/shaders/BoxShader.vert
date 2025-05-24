#version 450
#define MAZE_SIZE 17 // Make this the same as MazeGenerator.hpp
#define MAZE_HEIGHT 2 // Make this the same as MazeGenerator.hpp
#extension GL_ARB_separate_shader_objects : enable

// The attributes associated with each vertex.
// Their type and location must match the definition given in the
// corresponding Vertex Descriptor, and in turn, with the CPP data structure
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;

// this defines the variable passed to the Fragment Shader
// the locations must match the one of its in variables
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec2 fragUV;

// Here the Uniform buffers are defined. In this case, the Transform matrices (Set 1, binding 0)
// are used. Note that the definition must match the one used in the CPP code
layout(set = 1, binding = 0) uniform MazeUniformBufferObject {
	mat4 mvpMat[MAZE_SIZE][MAZE_SIZE][MAZE_HEIGHT];
	mat4 mMat[MAZE_SIZE][MAZE_SIZE][MAZE_HEIGHT];
	mat4 nMat[MAZE_SIZE][MAZE_SIZE][MAZE_HEIGHT];
} mazeUbo;



// Here the shader simply computes clipping coordinates, and passes to the Fragment Shader
// the position of the point in World Space, the transformed direction of the normal vector,
// and the untouched (but interpolated) UV coordinates
void main() {
	int i = gl_InstanceIndex;
	int r = gl_InstanceIndex / (MAZE_SIZE*2);
	int c = (gl_InstanceIndex % (MAZE_SIZE*2)) /2;
	int h = gl_InstanceIndex % (MAZE_HEIGHT);
	// Clipping coordinates must be returned in global variable gl_Posision
	gl_Position = mazeUbo.mvpMat[r][c][h] * vec4(inPosition, 1.0);
	// Here the value of the out variables passed to the Fragment shader are computed
	fragPos = (mazeUbo.mMat[r][c][h] * vec4(inPosition, 1.0)).xyz;
	fragNorm = (mazeUbo.nMat[r][c][h] * vec4(inNorm, 0.0)).xyz;
	fragUV = inUV;
}