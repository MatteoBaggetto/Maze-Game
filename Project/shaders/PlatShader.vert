#version 450
#define PLATFORM_NUMBER 2
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

struct UniformBufferObject {
	mat4 mvpMat;
	mat4 mMat;
	mat4 nMat;
};

// Here the Uniform buffers are defined. In this case, the Transform matrices (Set 1, binding 0)
// are used. Note that the definition must match the one used in the CPP code
layout(set = 1, binding = 0) uniform PlatformUniformBufferObject {
	UniformBufferObject ubo[PLATFORM_NUMBER];
} platUbo;

// Here the shader simply computes clipping coordinates, and passes to the Fragment Shader
// the position of the point in World Space, the transformed direction of the normal vector,
// and the untouched (but interpolated) UV coordinates
void main() {

    int i = gl_InstanceIndex;

	// Clipping coordinates must be returned in global variable gl_Posision
	gl_Position = platUbo.ubo[i].mvpMat * vec4(inPosition, 1.0);
	// Here the value of the out variables passed to the Fragment shader are computed
	fragPos = (platUbo.ubo[i].mMat * vec4(inPosition, 1.0)).xyz;
	fragNorm = (platUbo.ubo[i].nMat * vec4(inNorm, 0.0)).xyz;
	fragUV = inUV;
}
