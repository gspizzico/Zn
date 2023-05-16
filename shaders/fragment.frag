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
    float   occlusion;   
} materialAttributes;

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec3 inBiTangent;
layout (location = 4) in vec2 inUV;

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

// GGX/Trowbridge-Reitz Normal Distribution
float D_GGX(float NdotH, float roughness)
{
    float alphaSquared = pow(roughness * roughness, 2);
    float NdotHSquared = NdotH * NdotH;
    float denominator = max(PI * pow(NdotHSquared * (alphaSquared - 1.0) + 1.0, 2.0), 0.000001);
    return alphaSquared / denominator;
}

// Smith Model
float G_Smith(float NdotV, float NdotL, float roughness) {

    float alpha = roughness * roughness;
    float G_V = NdotV / max((NdotV * (1.0 - alpha) + alpha), 0.000001);
    float G_L = NdotL / max((NdotL * (1.0 - alpha) + alpha), 0.000001);
    return G_V * G_L;
}

vec3 conductor_frenel(vec3 F0, float specular, float HdotV) {
// ABS OR MAX?
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
    float Distribution = D_GGX(NdotH, roughness);
    float V = G / (4 * NdotL * NdotH);
    return V * Distribution;
}

vec4 diffuse_brdf(vec4 color)
{
    return (1.0 / PI) * color;
}

vec3 PBR(vec3 lightDirection, vec3 normal, vec3 viewDirection, float roughness, float metalness)
{
    vec3 HalfWay = normalize(lightDirection + viewDirection);

    float NdotL = max(dot(normal, lightDirection), 0.0);
    float NdotV = max(dot(normal, viewDirection), 0.0);
    float HdotV = max(dot(HalfWay, viewDirection), 0.0);
    float HdotN = max(dot(HalfWay, normal), 0.0);

    vec3 Ks = conductor_frenel(materialAttributes.baseColor.xyz, 1.0, HdotV);
    vec3 Kd = vec3(1.0) - Ks * (1.0 - metalness + 0.04);

    vec3 Lambert = materialAttributes.baseColor.xyz / PI;

    vec3 cookTorranceNumerator = 
        D_GGX(HdotN, roughness) *
        G_Smith(NdotV, NdotL, roughness) *
        Ks;

    float cookTorranceDenominator = max(4.0 * NdotV * NdotL, 0.000001);
    vec3 cookTorrance = cookTorranceNumerator / cookTorranceDenominator;

    vec3 BRDF = Kd * Lambert + cookTorrance;

    return BRDF * NdotL;
}

void main() 
{
    vec3 normal = normalize(texture(sampler_pbr_textures[PBR_INDEX_NORMAL], inUV).rgb * 2.0 - 1.0);
    vec3 tangent = normalize(inTangent.xyz);
    vec3 bitangent = cross(normalize(inNormal), tangent) * 1;

    if (gl_FrontFacing == false)
    {
        normal *= -1.0;
        tangent *= -1.0;
        bitangent *= -1.0;
    }

    mat3 TBN = transpose(mat3(
        tangent,
        bitangent,
        normalize(inNormal)
    ));

    vec3 V = normalize( TBN * ( camera.position.xyz - inPosition.xyz ) );

    float metalness = materialAttributes.metalness;
    float roughness = materialAttributes.roughness;
    float occlusion = materialAttributes.occlusion;

    vec3 metalness_sample = texture(sampler_pbr_textures[PBR_INDEX_METALNESS], inUV).rgb;

    // Green contains roughness
    roughness *= metalness_sample.g;

    // Blue contains metalness
    metalness *= metalness_sample.b;

    // Red channel contains occlusion
    occlusion *= texture(sampler_pbr_textures[PBR_INDEX_OCCLUSION], inUV).r;

    vec4 baseColor = texture(sampler_pbr_textures[PBR_INDEX_BASECOLOR], inUV) * materialAttributes.baseColor;

    baseColor.rgb = decode_srgb(baseColor.rgb);

    vec3 finalColor =  baseColor.rgb;

    for (uint i = 0; i < lighting.num_directional_lights; ++i) 
    {   
        const DirectionalLight light = lighting.directional_lights[i];
        vec3 direction = normalize(light.direction.xyz);
        finalColor += PBR(direction, normal, V, roughness, metalness) * light.color.xyz * light.intensity;
    }

    outColor = vec4(encode_srgb(finalColor), baseColor.a);
}