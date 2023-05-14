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

layout(set = 0, binding = 0) uniform CameraBuffer
{
    vec4 position;
	mat4 view;
	mat4 projection;
	mat4 view_projection;
} camera;

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

#define PI 3.1415926538

layout(set = 1, binding = 0) uniform sampler2D sampler_pbr_textures[PBR_NUM_TEXTURES];

layout(set = 1, binding = 1) uniform UBOMaterialAttributes
{
    vec4    baseColor;
    float   metalness;
    float   roughness;
    float   alphaCutoff;
    vec3    emissive;   
} materialAttributes;

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec2 inUV;

layout(location = 0) out vec4 outColor;

vec3 decode_srgb( vec3 c ) {
    vec3 result;
    if ( c.r <= 0.04045) {
        result.r = c.r / 12.92;
    } else {
        result.r = pow( ( c.r + 0.055 ) / 1.055, 2.4 );
    }

    if ( c.g <= 0.04045) {
        result.g = c.g / 12.92;
    } else {
        result.g = pow( ( c.g + 0.055 ) / 1.055, 2.4 );
    }

    if ( c.b <= 0.04045) {
        result.b = c.b / 12.92;
    } else {
        result.b = pow( ( c.b + 0.055 ) / 1.055, 2.4 );
    }

    return clamp( result, 0.0, 1.0 );
}

vec3 encode_srgb( vec3 c ) {
    vec3 result;
    if ( c.r <= 0.0031308) {
        result.r = c.r * 12.92;
    } else {
        result.r = 1.055 * pow( c.r, 1.0 / 2.4 ) - 0.055;
    }

    if ( c.g <= 0.0031308) {
        result.g = c.g * 12.92;
    } else {
        result.g = 1.055 * pow( c.g, 1.0 / 2.4 ) - 0.055;
    }

    if ( c.b <= 0.0031308) {
        result.b = c.b * 12.92;
    } else {
        result.b = 1.055 * pow( c.b, 1.0 / 2.4 ) - 0.055;
    }

    return clamp( result, 0.0, 1.0 );
}

float D_GGX(float NdotH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom);
}

float G_Smith(float NdotV, float NdotL, float roughness) {
    float alpha = roughness*roughness;
    float G_V = NdotV / (NdotV * (1.0 - alpha) + alpha);
    float G_L = NdotL / (NdotL * (1.0 - alpha) + alpha);
    return G_V * G_L;
}

vec3 conductor_frenel(vec3 F0, float specular, float HdotV) {
    return specular * (F0 + (1.0 - F0) * (1 - pow(abs(HdotV), 5.0)));
}

vec3 fresnel_mix(vec3 baseColor, float specular, float HdotV)
{
    //f0 in the formula notation refers to the value derived from ior = 1.5
    float f0 = 0.04; //pow((1.0f - ior) / (1.0 + ior), 2);
    float fr = f0 + (1.0 - f0) * (1 - pow(abs(HdotV), 5.0));
    return mix(baseColor, vec3(specular), fr);
}

float specular_brdf(float NdotV, float NdotH, float NdotL, float roughness)
{
    float G = G_Smith(NdotV, NdotL, roughness);
    float D = D_GGX(NdotH, roughness);
    float V = G / (4 * NdotL * NdotH);
    return V * D;
}

vec4 diffuse_brdf(vec4 color)
{
    return (1.0 / PI) * color;
}

vec3 ComputeBRDF(vec3 lightDirection, vec3 normal, vec3 viewDirection)
{
    vec3 halfWay = normalize(lightDirection + viewDirection);

    float NdotL = dot(normal, lightDirection);
    float NdotV = dot(normal, viewDirection);
    float HdotN = dot(halfWay, normal);
    float HdotV = dot(halfWay, viewDirection);

    float specular = specular_brdf(NdotV, HdotN, NdotL, materialAttributes.roughness);
    vec4 diffuse = diffuse_brdf(materialAttributes.baseColor);

    vec3 dielectric_brdf = fresnel_mix(materialAttributes.baseColor.xyz, specular, HdotV);
    vec3 metal_brdf = specular * conductor_frenel(materialAttributes.baseColor.xyz, specular, HdotV);    

    // return mix(dielectric_brdf, metal_brdf, materialAttributes.metalness);
    return dielectric_brdf;
}

void main() 
{
    vec3 normal = normalize(texture(sampler_pbr_textures[PBR_INDEX_NORMAL], inUV).rgb * 2.0 - 1.0);
    vec3 tangent = normalize(inTangent.xyz);
    vec3 bitangent = cross(normalize(inNormal), tangent) * 1;

    mat3 TBN = transpose(mat3(
        tangent,
        bitangent,
        normalize(inNormal)
    ));

    vec3 V = normalize( TBN * ( camera.position.xyz - inPosition.xyz ) );

    vec3 baseColor = decode_srgb(texture(sampler_pbr_textures[PBR_INDEX_BASECOLOR], inUV).rgb);

    vec3 finalColor =  baseColor;

    for (uint i = 0; i < lighting.num_directional_lights; ++i) 
    {   
        const DirectionalLight light = lighting.directional_lights[i];
        // vec3 direction = normalize(-lighting.directional_lights[i].direction.xyz);
        vec3 direction = normalize(TBN * light.direction.xyz - inPosition.xyz);
        finalColor += (ComputeBRDF(direction, normal, V) * light.color.xyz * light.intensity);
    }
    
//    for (uint i = 0; i < lighting.num_point_lights; ++i) 
//    {
//        finalColor += applyPointLight(lighting.point_lights[i], normal, inPosition, viewDir);
//    }
     
    // vec3 ambientComponent = lighting.ambient_light.color.xyz * lighting.ambient_light.intensity;
    //  
    // finalColor += ambientComponent;

    outColor = vec4(encode_srgb(finalColor), 1.0);
}