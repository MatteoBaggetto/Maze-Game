#version 450
#define KEYS_NUMBER 3
#define WALL_LIGHTS_NUMBER 6
#define PI 3.14159265358979323846
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
} gubo;


layout(set = 1, binding = 1) uniform sampler2D texDiff;
layout(set = 1, binding = 2) uniform sampler2D texSpec;


vec3 point_light_dir(vec3 lightPos) {
	// Point light - direction vector
	return normalize(lightPos-fragPos);
}

vec3 point_light_color(vec3 lightPos, vec4 lightColor, float decayFactor) {
	// Point light - color
	float g = lightColor.a;
	float distance = length(lightPos - fragPos);
    float decay = pow(g / distance, decayFactor);
	return lightColor.rgb*decay;
}



float min(float n1, float n2, float n3){

    return min(n1, min(n2, n3));
}

// :)

// The main shader, implementing a simple Blinn + Lambert + constant Ambient BRDF model

void main() {
	vec3 Norm = normalize(fragNorm);
	vec3 EyeDir = normalize(gubo.eyePos - fragPos);

	float F0 = 0.9f;
    float Ro = 0.2f;

	vec3 DiffuseOriginalColor = texture(texDiff, fragUV).rgb;
	vec3 SpecularOriginalColor = texture(texSpec, fragUV).rgb;
	// Hand Light

	vec3 handLightColorComputed = point_light_color(gubo.handLightPos, gubo.handLightColor, gubo.handLightDecayFactor);
	vec3 handLightDir = point_light_dir(gubo.handLightPos);
	vec3 handLightHalfVec = normalize(handLightDir + EyeDir);

    float halfDotNHandLight = clamp(dot(Norm, handLightHalfVec), 0.00001f, 1.0f);
    float eyeDirDotNHandLight = clamp(dot(EyeDir, Norm), 0.00001f, 1.0f);
    float eyeDirDotHalfHandLight = clamp(dot(EyeDir, handLightHalfVec), 0.00001f, 1.0f);
    float lightDirDotNHandLight = clamp(dot(handLightDir, handLightHalfVec), 0.00001f, 1.0f);

	// G: geometric term
	float GhandLight = min(1.0f, 2.0f * halfDotNHandLight * eyeDirDotNHandLight / eyeDirDotHalfHandLight, 2.0 * halfDotNHandLight * lightDirDotNHandLight / eyeDirDotHalfHandLight);
	// F: fresnel - how light responds to the angle of the viewer
    float FhandLight = F0 + (1.0f - F0) * pow(1.0f - eyeDirDotHalfHandLight, 5.0f);

	float Den = PI * pow(Ro,2.0f) * pow(halfDotNHandLight, 4.0f);
	float ExpDen = pow(halfDotNHandLight, 2.0f)*pow(Ro,2.0f);
	float ExpNum = - (1.0f - pow(halfDotNHandLight,2.0f));

	// D: distribution term - takes into account the surface roughness
	float DhandLight = exp(ExpNum/ExpDen) / Den;

	vec3 handLightDiffuse = DiffuseOriginalColor * max(dot(Norm, handLightDir),0.0);
	
	vec3 handLightSpecular = SpecularOriginalColor * (DhandLight*FhandLight*GhandLight)/(4.0f*eyeDirDotNHandLight);

	vec3 handLightFinalEffect  = (0.7 * handLightDiffuse + (1-0.7) * handLightSpecular) * handLightColorComputed.rgb;
	

	// Wall lamp lights
	int i=0;
	vec3 wallLightFinalEffectOverall = vec3(0.0f);
	vec3 wallLightColorComputed;
	vec3 wallLightDir;
	vec3 wallLightHalfVec;
	vec3 wallLightDiffuse;
	vec3 wallLightSpecular;
	vec3 wallLightFinalEffect;
	float halfDotNWallLight;
    float eyeDirDotNWallLight;
    float eyeDirDotHalfWallLight;
    float lightDirDotNWallLight;

	float GWallLight;
    float FWallLight;

	float DWallLight;
	while(i<WALL_LIGHTS_NUMBER) {
		wallLightColorComputed = point_light_color(vec3(gubo.wallLampPos[i]), gubo.wallLampColor, gubo.wallLampDecayFactor);
		wallLightDir = point_light_dir(vec3(gubo.wallLampPos[i]));
		wallLightHalfVec = normalize(wallLightDir + EyeDir);

		halfDotNWallLight = clamp(dot(Norm, wallLightHalfVec), 0.00001f, 1.0f);
		eyeDirDotNWallLight = clamp(dot(EyeDir, Norm), 0.00001f, 1.0f);
		eyeDirDotHalfWallLight = clamp(dot(EyeDir, wallLightHalfVec), 0.00001f, 1.0f);
		lightDirDotNWallLight = clamp(dot(wallLightDir, wallLightHalfVec), 0.00001f, 1.0f);

		GWallLight = min(1.0f, 2.0f * halfDotNWallLight * eyeDirDotNWallLight / eyeDirDotHalfWallLight, 2.0 * halfDotNWallLight * lightDirDotNWallLight / eyeDirDotHalfWallLight);
		FWallLight = F0 + (1.0f - F0) * pow(1.0f - eyeDirDotHalfWallLight, 5.0f);

		Den = PI * pow(Ro,2.0f) * pow(halfDotNWallLight, 4.0f);
		ExpDen = pow(halfDotNWallLight, 2.0f)*pow(Ro,2.0f);
		ExpNum = - (1.0f - pow(halfDotNWallLight,2.0f));

		DWallLight = exp(ExpNum/ExpDen) / Den;

		wallLightDiffuse = DiffuseOriginalColor * max(dot(Norm, wallLightDir),0.0);
		wallLightSpecular = SpecularOriginalColor * (DWallLight*FWallLight*GWallLight)/(4.0f*eyeDirDotNWallLight);
		
		wallLightFinalEffect  = (0.7f * wallLightDiffuse + (1-0.7f) * wallLightSpecular) * wallLightColorComputed.rgb;
		wallLightFinalEffectOverall += wallLightFinalEffect;
		i++;
	}

	// Final lights

	outColor = vec4(handLightFinalEffect + wallLightFinalEffectOverall, 1.0f);
}