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

vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 Md, vec3 Ms) {
/* This BRDF should perform toon shading with the following charcteristics:
Lets call cos(alpha) the cosine of the angle between the normal vector and the light vector,
and cos(beta) the cosine of the angle between the specular reflection vector and the viewer direction.
The toon will return the sum of a pecentage of the diffuse and a percentage of the specular colors,
according to the following proportions:

Diffuse -   0% if cos(alpha) is zero or negative
        -   gradient between 0% and 15% if 0 < cos(alpha) <= 0.1
		-  15% if 0.1 < cos(alpha) <= 0.7
		-   gradient between 15% and 100% if 0.7 < cos(alpha) <= 0.8
		- 100% if cos(alpha) > 0.8

Specular -   0% if cos(beta) <= 0.9
		 -   gradient between 0% and 100% if 0.9 < cos(alpha) <= 0.95
		 - 100% if cos(beta > 0.95)
		 
Paramters:
	V	- Viewer direction
	N	- Normal vector direction
	L	- Light direction
	Md	- Diffuse color
	Ms	- Specular color
*/

	/*diffuse*/
	float perc;
	float cosangle;

	cosangle = clamp(dot(N, L),0.0,1.0);


	if(cosangle <= 0.0f)
		perc = 0.0f;

	else if(cosangle > 0.0f && cosangle <= 0.1f)
		perc = clamp((cosangle-0.0f)/(0.1f-0.0f), 0.0f, 0.15f);
	

	else if(cosangle > 0.1f && cosangle <= 0.7f)
		perc = 0.15f;

	else if(cosangle > 0.7f && cosangle <= 0.8f)
		perc = clamp((cosangle-0.7f)/(0.8f-0.7f), 0.15f, 1.0f);

	else
		perc = 1.0f;

	
	

	/*specular*/
	float cosangle2;
	float perc2;

	cosangle2 = clamp(dot(V, -reflect(L, N)),0.0,1.0);


	if(cosangle2 <= 0.9f)
		perc2 = 0.0f;

	else if(cosangle2 > 0.9f && cosangle2 <= 0.95f)
		perc2 = clamp((cosangle2-0.9f)/(0.95f-0.9f), 0.0f, 1.0f);

	else
		perc2 = 1.0f;



	/*final*/
	vec3 Diffuse = perc * Md;
	vec3 Specular = perc2 * Ms; 
	
	return (Diffuse+Specular);
}

void main() {
	vec3 Norm = normalize(fragNorm);
	vec3 EyeDir = normalize(gubo.eyePos - fragPos);
	vec3 Albedo = texture(texDiff, fragUV).rgb;

	//vec3 specular = texture(texSpec, fragUV).rgb;
	vec3 specular = vec3(1.0f, 1.0f, 1.0f);

	vec3 handLightDir = light_dir(gubo.handLightPos);
	vec3 handLightColorComputed = point_light_color(gubo.handLightPos, gubo.handLightColor, gubo.handLightDecayFactor);

	vec3 handLightFinalEffect  = handLightColorComputed.rgb * BRDF(EyeDir, Norm, handLightDir, Albedo, specular);

	// A special type of non-uniform ambient color, invented for this course
	const vec3 cxp = vec3(0.5,0.5,0.5) * 0.25;
	const vec3 cxn = vec3(0.5,0.5,0.5) * 0.25;
	const vec3 cyp = vec3(0.3,1.0,1.0) * 0.15; // sky
	const vec3 cyn = vec3(0.5,0.5,0.5) * 0.25;
	const vec3 czp = vec3(0.5,0.5,0.5) * 0.25;
	const vec3 czn = vec3(0.5,0.5,0.5) * 0.25;
	
	vec3 Ambient =((Norm.x > 0 ? cxp : cxn) * (Norm.x * Norm.x) +
				   (Norm.y > 0 ? cyp : cyn) * (Norm.y * Norm.y) +
				   (Norm.z > 0 ? czp : czn) * (Norm.z * Norm.z)) * Albedo;
	


	int i=0;
	vec3 wallLightFinalEffectOverall = vec3(0.0f);
	vec3 wallLightColorComputed;
	vec3 wallLightDir;
	vec3 wallLightFinalEffect;
	while(i<WALL_LIGHTS_NUMBER) {
		wallLightColorComputed = point_light_color(vec3(gubo.wallLampPos[i]), gubo.wallLampColor, gubo.wallLampDecayFactor);
		wallLightDir = light_dir(vec3(gubo.wallLampPos[i]));
		
		wallLightFinalEffect = wallLightColorComputed.rgb *BRDF(EyeDir, Norm, wallLightDir, Albedo, specular); 

		wallLightFinalEffectOverall += wallLightFinalEffect;
		i++;
	}

	//SPOT LIGHT CUP 

	vec3 cupLightDir;
	vec3 cupLightColorComputed;
	vec3 cupLightFinalEffect;

	cupLightDir = light_dir(gubo.cupLightPos);
	cupLightColorComputed = spot_light_color(gubo.cupLightPos, gubo.cupLightColor, gubo.cupLightDecayFactor, cupLightDir, gubo.cupCIN, gubo.cupCOUT);

	cupLightFinalEffect  = cupLightColorComputed.rgb * BRDF(EyeDir, Norm, cupLightDir, Albedo, specular);



	
	outColor = vec4(wallLightFinalEffectOverall+handLightFinalEffect + Ambient + cupLightFinalEffect, 1.0f);
}
