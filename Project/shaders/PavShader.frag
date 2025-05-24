#version 450
#define KEYS_NUMBER 3
#define WALL_LIGHTS_NUMBER 6
#extension GL_ARB_separate_shader_objects : enable

// this defines the variable received from the Vertex Shader
// the locations must match the one of its out variables
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;

// This defines the color computed by this shader. Generally is always location 0.
layout(location = 0) out vec4 outColor;

// Here the Uniform buffers are defined. In this case, the Global Uniforms of Set 0
// The texture of Set 1 (binding 1), and the Material parameters (Set 1, binding 2)
// are used. Note that each definition must match the one used in the CPP code
layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
	vec3 eyePos;
	vec3 handLightPos;
	vec4 handLightColor;
	float handLightDecayFactor;
	vec4 wallLampPos[WALL_LIGHTS_NUMBER];
	vec4 wallLampColor;
	float wallLampDecayFactor;

	vec3 cupLightPos;
	vec4 cupLightColor;
	vec3 cupLightDir;
	float cupLightDecayFactor;
	float cupCIN;
	float cupCOUT;

} gubo;


layout(set = 1, binding = 1) uniform sampler2D texDiff;
layout(set = 1, binding = 2) uniform sampler2D texSpec;

layout(set = 1, binding = 3) uniform PavementParametersUniformBufferObject {
	float uScale;
	float vScale;
	float blinnGamma;
	float balanceDiffuseSpecular;
} pavparUBO;

vec3 light_dir(vec3 lightPos) {
	// Point light - direction vector
	return normalize(lightPos-fragPos);
}

vec3 point_light_color(vec3 lightPos, vec4 lightColor, float decayFactor) {
	// Point light - color
	float distance = length(lightPos - fragPos);
	return lightColor.rgb*pow(lightColor.a / distance, decayFactor);
}


vec3 spot_light_color(vec3 lightPos, vec4 lightColor, float decayFactor, vec3 lightDir, float CIN, float COUT) {
	float distance = length(lightPos - fragPos);
	return lightColor.rgb* (pow (lightColor.a / distance , decayFactor)) * clamp( (dot(normalize(lightPos-fragPos), lightDir) - COUT) /(CIN-COUT),0.0f, 1.0f) ;
}

// :)

// The main shader, implementing a simple Blinn + Lambert + constant Ambient BRDF model

void main() {
	vec3 Norm = normalize(fragNorm);
	vec3 EyeDir = normalize(gubo.eyePos - fragPos);

	vec3 DiffuseOriginalColor = texture(texDiff, fragUV).rgb;
	vec3 SpecularOriginalColor = texture(texSpec, fragUV).rgb;

	// Hand Light

	vec3 handLightColorComputed = point_light_color(gubo.handLightPos, gubo.handLightColor, gubo.handLightDecayFactor);
	vec3 handLightDir = light_dir(gubo.handLightPos);
	vec3 handLightHalfVec = normalize(handLightDir + EyeDir);

	vec3 handLightDiffuse = DiffuseOriginalColor * max(dot(Norm, handLightDir),0.0);
	vec3 handLightSpecular = SpecularOriginalColor * pow(max(dot(Norm, handLightHalfVec), 0.0), pavparUBO.blinnGamma);
	
	vec3 handLightFinalEffect  = (pavparUBO.balanceDiffuseSpecular * handLightDiffuse + (1-pavparUBO.balanceDiffuseSpecular) * handLightSpecular) * handLightColorComputed.rgb;
	
	//spot light for cup

	vec3 cupLightDir = light_dir(gubo.cupLightPos);
	vec3 cupLightColorComputed = spot_light_color(gubo.cupLightPos, gubo.cupLightColor, gubo.cupLightDecayFactor, cupLightDir, gubo.cupCIN, gubo.cupCOUT);
	
	vec3 cupLightHalfVec = normalize(cupLightDir + EyeDir);

	vec3 cupLightDiffuse = DiffuseOriginalColor * max(dot(Norm, cupLightDir),0.0);
	vec3 cupLightSpecular = SpecularOriginalColor * pow(max(dot(Norm, cupLightHalfVec), 0.0), pavparUBO.blinnGamma);
	
	vec3 cupLightFinalEffect  = (pavparUBO.balanceDiffuseSpecular * cupLightDiffuse + (1-pavparUBO.balanceDiffuseSpecular) * cupLightSpecular) * cupLightColorComputed.rgb;
	







	// Wall lamp lights
	int i=0;
	vec3 wallLightFinalEffectOverall = vec3(0.0f);
	vec3 wallLightColorComputed;
	vec3 wallLightDir;
	vec3 wallLightHalfVec;
	vec3 wallLightDiffuse;
	vec3 wallLightSpecular;
	vec3 wallLightFinalEffect;
	while(i<WALL_LIGHTS_NUMBER) {
		wallLightColorComputed = point_light_color(vec3(gubo.wallLampPos[i]), gubo.wallLampColor, gubo.wallLampDecayFactor);
		wallLightDir = light_dir(vec3(gubo.wallLampPos[i]));
		wallLightHalfVec = normalize(wallLightDir + EyeDir);

		wallLightDiffuse = DiffuseOriginalColor * max(dot(Norm, wallLightDir),0.0);
		wallLightSpecular =  SpecularOriginalColor * pow(max(dot(Norm, wallLightHalfVec), 0.0), pavparUBO.blinnGamma);
		
		wallLightFinalEffect  = (pavparUBO.balanceDiffuseSpecular * wallLightDiffuse + (1-pavparUBO.balanceDiffuseSpecular) * wallLightSpecular) * wallLightColorComputed.rgb;
		wallLightFinalEffectOverall += wallLightFinalEffect;
		i++;
	}


	// Final lights

	outColor = vec4(cupLightFinalEffect+handLightFinalEffect +	wallLightFinalEffectOverall, 1.0f);
}