#version 450

struct PointLight
{
    vec4 position;
    vec4 color;
    float intensity;
    float constant_attenuation;
    float linear_attenuation;
    float quadratic_attenuation;
};

struct DirectionalLight
{
    vec4 direction;
    vec4 color;
    float intensity;
};

struct AmbientLight
{
    vec4 color;
    float intensity;
};

#define MAX_POINT_LIGHTS 16
#define MAX_DIRECTIONAL_LIGHTS 1

layout (std140, set = 0, binding = 1) uniform LightingUniforms
{
    PointLight point_lights[MAX_POINT_LIGHTS];
	DirectionalLight directional_lights[MAX_DIRECTIONAL_LIGHTS];
    AmbientLight ambient_light;
    uint num_point_lights;
    uint num_directional_lights;
} lighting;

#define PBR_INDEX_BASECOLOR 0
#define PBR_INDEX_METALNESS 1
#define PBR_INDEX_NORMAL    2
#define PBR_INDEX_OCCLUSION 3
#define PBR_INDEX_EMISSIVE  4

#define PBR_NUM_TEXTURES    5

layout(set = 1, binding = 0) uniform sampler2D sampler_pbr_textures[PBR_NUM_TEXTURES];

layout(set = 1, binding = 1) uniform UBOMaterialAttributes
{
    vec4    baseColor;
    float   metalness;
    float   roughness;
    float   alphaCutoff;
    vec3    emissive;   
} materialAttributes;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout(location = 0) out vec4 outColor;

vec3 applyDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir) 
{
    vec3 lightDir = normalize(-light.direction.xyz);
    float diffuseFactor = max(dot(normal, lightDir), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specularFactor = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    return (diffuseFactor + specularFactor) * light.color.xyz * light.intensity;
}

vec3 applyPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) 
{
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    float distance = length(light.position.xyz - fragPos);
    float diffuseFactor = max(dot(normal, lightDir), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specularFactor = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    float attenuation = 1.0 / (light.constant_attenuation + light.linear_attenuation * distance + light.quadratic_attenuation * (distance * distance));

    return (diffuseFactor + specularFactor) * light.color.xyz * light.intensity * attenuation;
}

void main() 
{
    vec3 viewDir = normalize(-inPosition);
    vec3 normal = normalize(inNormal);

    vec3 finalColor =  texture(sampler_pbr_textures[PBR_INDEX_BASECOLOR], inUV).xyz * materialAttributes.baseColor.xyz;

    for (uint i = 0; i < lighting.num_directional_lights; ++i) 
    {
        finalColor += applyDirectionalLight(lighting.directional_lights[i], normal, viewDir);
    }
    
    for (uint i = 0; i < lighting.num_point_lights; ++i) 
    {
        finalColor += applyPointLight(lighting.point_lights[i], normal, inPosition, viewDir);
    }
     
    vec3 ambientComponent = lighting.ambient_light.color.xyz * lighting.ambient_light.intensity;
     
    finalColor += ambientComponent;

    outColor = vec4(finalColor, 1.0);
}