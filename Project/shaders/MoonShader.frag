#version 450
#define KEYS_NUMBER 3
#define WALL_LIGHTS_NUMBER 6
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;


layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform sampler2D texSampler;


layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
	vec3 eyePos;
	vec3 handLightPos;
	vec4 handLightColor;
	float handLightDecayFactor;
	vec4 wallLampPos[WALL_LIGHTS_NUMBER];
	vec4 wallLampColor;
	float wallLampDecayFactor;
} gubo;



layout(set = 1, binding = 1) uniform sampler2D texDiff;


void main() {

	vec3 DiffuseOriginalColor = texture(texDiff, fragUV).rgb;
    vec3 EmitColor = vec3(0.2f, 0.2f,0.0f);
	
	outColor = vec4(DiffuseOriginalColor + EmitColor , 1.0f);

}
