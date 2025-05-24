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

	vec3 cupLightPos;
	vec4 cupLightColor;
	vec3 cupLightDir;
	float cupLightDecayFactor;
	float cupCIN;
	float cupCOUT;

} gubo;



layout(set = 1, binding = 1) uniform sampler2D texDiff;
layout(set = 1, binding = 2) uniform sampler2D texSpec;


vec3 light_dir(vec3 lightPos) {
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

vec3 spot_light_color(vec3 lightPos, vec4 lightColor, float decayFactor, vec3 lightDir, float CIN, float COUT) {
	float distance = length(lightPos - fragPos);
	return lightColor.rgb* (pow (lightColor.a / distance , decayFactor)) * clamp( (dot(normalize(lightPos-fragPos), lightDir) - COUT) /(CIN-COUT),0.0f, 1.0f) ;
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

	// RO: 1 = rough, 0 = smooth, very reflective
    float Ro = 0.2f;

	vec3 DiffuseOriginalColor = texture(texDiff, fragUV).rgb;
	vec3 SpecularOriginalColor = texture(texSpec, fragUV).rgb;
	// Hand Light

	vec3 handLightColorComputed = point_light_color(gubo.handLightPos, gubo.handLightColor, gubo.handLightDecayFactor);
	vec3 handLightDir = light_dir(gubo.handLightPos);
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
	
	//CUP SPOT LIGHT

	vec3 cupLightColorComputed;
	vec3 cupLightDir;
	vec3 cupLightHalfVec;
	vec3 cupLightDiffuse;
	vec3 cupLightSpecular;
	vec3 cupLightFinalEffect;
	float halfDotNCupLight;
    float eyeDirDotNCupLight;
    float eyeDirDotHalfCupLight;
    float lightDirDotNCupLight;

	float GCupLight;
    float FCupLight;

	float DCupLight;
	
	// Spot light near the cup
	cupLightDir = light_dir(gubo.cupLightPos);
	cupLightColorComputed = spot_light_color(gubo.cupLightPos, gubo.cupLightColor, gubo.cupLightDecayFactor, cupLightDir, gubo.cupCIN, gubo.cupCOUT);
			
	
	
	cupLightHalfVec = normalize(cupLightDir + EyeDir);

	halfDotNCupLight = clamp(dot(Norm, cupLightHalfVec), 0.00001f, 1.0f);
	eyeDirDotNCupLight = clamp(dot(EyeDir, Norm), 0.00001f, 1.0f);
	eyeDirDotHalfCupLight = clamp(dot(EyeDir, cupLightHalfVec), 0.00001f, 1.0f);
	lightDirDotNCupLight = clamp(dot(cupLightDir, cupLightHalfVec), 0.00001f, 1.0f);

	GCupLight = min(1.0f, 2.0f * halfDotNCupLight * eyeDirDotNCupLight / eyeDirDotHalfCupLight, 2.0 * halfDotNCupLight * lightDirDotNCupLight / eyeDirDotHalfCupLight);
	FCupLight = F0 + (1.0f - F0) * pow(1.0f - eyeDirDotHalfCupLight, 5.0f);

	Den = PI * pow(Ro,2.0f) * pow(halfDotNCupLight, 4.0f);
	ExpDen = pow(halfDotNCupLight, 2.0f)*pow(Ro,2.0f);
	ExpNum = - (1.0f - pow(halfDotNCupLight,2.0f));

	DCupLight = exp(ExpNum/ExpDen) / Den;

	cupLightDiffuse = DiffuseOriginalColor * max(dot(Norm, cupLightDir),0.0);
	cupLightSpecular = SpecularOriginalColor * (DCupLight*FCupLight*GCupLight)/(4.0f*eyeDirDotNCupLight);
		
	cupLightFinalEffect  = (0.7f * cupLightDiffuse + (1-0.7f) * cupLightSpecular) * cupLightColorComputed.rgb;
	
	

	// Final lights

	outColor = vec4(cupLightFinalEffect+handLightFinalEffect, 1.0f);
}