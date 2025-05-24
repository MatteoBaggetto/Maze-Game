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


layout(set = 1, binding = 4) uniform BoxParametersUniformBufferObject {
	float blinnGamma;
	float balanceDiffuseSpecular;
	float ambientFactor;
} boxparUBO;

layout(set = 1, binding = 1) uniform sampler2D texDiff;
layout(set = 1, binding = 2) uniform sampler2D texSpec;
layout(set = 1, binding = 3) uniform sampler2D texAMB;


vec3 point_light_dir(vec3 lightPos) {
	// Point light - direction vector
	return normalize(lightPos-fragPos);
}

vec3 point_light_color(vec3 lightPos, vec4 lightColor, float decayFactor) {
	// Point light - color
	float distance = length(lightPos - fragPos);
	return lightColor.rgb*pow(lightColor.a / distance, decayFactor);
}

// vec3 point_light_color(vec3 lightPos, vec4 lightColor, float decayFactor) {
// 	// Point light - color
// 	float g = lightColor.a;
// 	float decay;
// 	float distance = length(lightPos - fragPos);
	
//  decay = pow(g / distance, decayFactor);
// 	return lightColor.rgb*decay;
// }


void main() {
    vec3 Norm = normalize(fragNorm);
	vec3 EyeDir = normalize(gubo.eyePos - fragPos);
    vec3 AmbientColor = texture(texAMB, fragUV).rgb;

    const vec3 cxp = vec3(0.5,0.5,0.3) * 0.25;
	const vec3 cxn = vec3(0.5,0.5,0.3) * 0.25;
	const vec3 cyp = vec3(0.3,0.8,1.0) * 0.15; // sky
	const vec3 cyn = vec3(0.5,0.5,0.3) * 0.25;
	const vec3 czp = vec3(0.5,0.5,0.3) * 0.25;
	const vec3 czn = vec3(0.5,0.5,0.3) * 0.25;
	
	vec3 Ambient =((Norm.x > 0 ? cxp : cxn) * (Norm.x * Norm.x) +
				   (Norm.y > 0 ? cyp : cyn) * (Norm.y * Norm.y) +
				   (Norm.z > 0 ? czp : czn) * (Norm.z * Norm.z)) * AmbientColor * boxparUBO.ambientFactor ;
	vec3 DiffuseOriginalColor = texture(texDiff, fragUV).rgb;
	vec3 SpecularOriginalColor = texture(texSpec, fragUV).rgb;
	// Hand Light

	vec3 handLightColorComputed = point_light_color(gubo.handLightPos, gubo.handLightColor, gubo.handLightDecayFactor);
	vec3 handLightDir = point_light_dir(gubo.handLightPos);
	vec3 handLightHalfVec = normalize(handLightDir + EyeDir);

	vec3 handLightDiffuse = DiffuseOriginalColor * max(dot(Norm, handLightDir),0.0);
	vec3 handLightSpecular = SpecularOriginalColor * pow(max(dot(Norm, handLightHalfVec), 0.0), boxparUBO.blinnGamma);
	
	vec3 handLightFinalEffect  = (boxparUBO.balanceDiffuseSpecular * handLightDiffuse + (1-boxparUBO.balanceDiffuseSpecular) * handLightSpecular) * handLightColorComputed.rgb;
	
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
		wallLightDir = point_light_dir(vec3(gubo.wallLampPos[i]));
		wallLightHalfVec = normalize(wallLightDir + EyeDir);

		wallLightDiffuse = DiffuseOriginalColor * max(dot(Norm, wallLightDir),0.0);
		wallLightSpecular =  SpecularOriginalColor * pow(max(dot(Norm, wallLightHalfVec), 0.0), boxparUBO.blinnGamma);
		
		wallLightFinalEffect  = (boxparUBO.balanceDiffuseSpecular * wallLightDiffuse + (1-boxparUBO.balanceDiffuseSpecular) * wallLightSpecular) * wallLightColorComputed.rgb;
		wallLightFinalEffectOverall += wallLightFinalEffect;
		i++;
	}


	//
	// Final lights
	outColor = vec4(handLightFinalEffect + wallLightFinalEffectOverall + Ambient, 1.0f);

}
