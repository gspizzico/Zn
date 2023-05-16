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

vec3 applyDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir) 
{
    vec3 lightDir = normalize(light.direction.xyz);
    float diffuseFactor = max(dot(normal, lightDir), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specularFactor = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    return (diffuseFactor + specularFactor) * light.color.xyz *  light.intensity;
}

vec3 applyPointLight(PointLight light, vec3 normal, vec4 fragPos, vec3 viewDir) 
{
    vec3 lightDir = normalize(light.position.xyz - fragPos.xyz);
    float distance = length(light.position.xyz - fragPos.xyz);
    float diffuseFactor = max(dot(normal, lightDir), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specularFactor = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    float attenuation = 1.0 / (light.constant_attenuation + light.linear_attenuation * distance + light.quadratic_attenuation * (distance * distance));

    return (diffuseFactor + specularFactor) * light.color.xyz * light.intensity * attenuation;
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

    vec3 baseColor = decode_srgb(texture(sampler_pbr_textures[PBR_INDEX_BASECOLOR], inUV).rgb);

    vec3 finalColor =  baseColor;

    for (uint i = 0; i < lighting.num_directional_lights; ++i) 
    {
        finalColor += applyDirectionalLight(lighting.directional_lights[i], normal, V);
    }
    
    for (uint i = 0; i < lighting.num_point_lights; ++i) 
    {
        finalColor += applyPointLight(lighting.point_lights[i], normal, inPosition, V);
    }
     
    vec3 ambientComponent = lighting.ambient_light.color.xyz * lighting.ambient_light.intensity;
     
    finalColor += ambientComponent;

    outColor = vec4(encode_srgb(finalColor), 1.0);
}