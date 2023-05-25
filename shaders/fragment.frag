#version 450

layout(set = 0, binding = 0) uniform CameraBuffer
{
    vec4 position;
	mat4 view;
	mat4 projection;
	mat4 view_projection;
} camera;

#define MAX_POINT_LIGHTS 16
#define MAX_DIRECTIONAL_LIGHTS 1

layout(std140, set = 0, binding = 1) uniform Light
{
	vec4 position;
    float intensity;
    float range;
} uLight;

#define PBR_INDEX_BASECOLOR 0
#define PBR_INDEX_METALNESS 1
#define PBR_INDEX_NORMAL    2
#define PBR_INDEX_OCCLUSION 3
#define PBR_INDEX_EMISSIVE  4

#define PBR_NUM_TEXTURES    5

#define PI 3.1415926538

layout(set = 1, binding = 0) uniform sampler2D uTexture[PBR_NUM_TEXTURES];

layout(set = 1, binding = 1) uniform UBOMaterialAttributes
{
    vec4    baseColor;
    float   metalness;
    float   roughness;
    float   alphaCutoff;
    vec3    emissive;   
    float   occlusion;   
} materialAttributes;

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec3 vBiTangent;
layout (location = 4) in vec2 vTexCoord;

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

float heaviside( float v ) {
    if ( v > 0.0 ) return 1.0;
    else return 0.0;
}

vec3 calculateNormal()
{
    vec3 normal = normalize( vNormal );
    vec3 tangent = normalize( vTangent );
    vec3 bitangent = normalize( vBiTangent );

    if (gl_FrontFacing == false)
    {
        tangent *= -1.0;
        bitangent *= -1.0;
        normal *= -1.0;
    }
    
    vec3 bump_normal = normalize( texture(uTexture[PBR_INDEX_NORMAL], vTexCoord).rgb * 2.0 - vec3(1.0));
    mat3 TBN = mat3(
        tangent,
        bitangent,
        normal
    );

    normal = normalize(TBN * normalize(bump_normal));
    return normal;
}

struct PBRParams
{
    vec3 V;
    vec3 L;
    vec3 H;
    vec3 N;

    float NdotL;
    float NdotV;
    float NdotH;
    float HdotL;
    float HdotV;

    float roughness;
    float metallic;

    vec4 srgbColor;
    vec3 baseColor;

    float lightDistance;
    float lightIntensity;
};

PBRParams InitializePBR()
{
    PBRParams params;

    params.V = normalize(camera.position.xyz - vPosition);
    params.L = normalize(uLight.position.xyz - vPosition);
    params.H = normalize(params.L + params.V);
    params.N = calculateNormal();

    params.NdotL = clamp(dot(params.N, params.L), 0.0, 1.0);
    params.NdotV = clamp(abs(dot(params.N, params.V)), 0.0, 1.0);
    params.NdotH = clamp(dot(params.N, params.H), 0.0, 1.0);
    params.HdotL = clamp(dot(params.H, params.L), 0.0, 1.0);
    params.HdotV = clamp(dot(params.H, params.V), 0.0, 1.0);

    params.roughness = materialAttributes.roughness;
    params.metallic = materialAttributes.metalness;
    
    vec3  rm = texture(uTexture[PBR_INDEX_METALNESS], vTexCoord).rgb;
    
    params.roughness *= rm.g;
    params.metallic *= rm.b;

    params.srgbColor = texture(uTexture[PBR_INDEX_BASECOLOR], vTexCoord) * materialAttributes.baseColor;
    params.baseColor = decode_srgb(params.srgbColor.rgb);

    params.lightDistance = length(uLight.position.xyz - vPosition);
    params.lightIntensity = uLight.intensity * max(min(1.0 - pow(params.lightDistance / uLight.range, 4.0), 1.0), 0.0) / pow(params.lightDistance, 2.0);

    return params;
}

// https://github.com/PacktPublishing/Mastering-Graphics-Programming-with-Vulkan/
void main()
{
    PBRParams params = InitializePBR();

    float alpha = pow(params.roughness, 2);

    float alpha_squared = alpha * alpha;
    float d_denom = (params.NdotH * params.NdotH) * ( alpha_squared - 1.0 ) + 1.0;
    float distribution = (alpha_squared * heaviside(params.NdotH) ) / (PI * d_denom * d_denom);

    vec3 material_colour = vec3(0.0);
    if(params.NdotL > 0.0 || params.NdotV > 0.0)
    {
        float visibility = (heaviside(params.HdotL) / (abs(params.NdotL) + sqrt(alpha_squared + (1.0 - alpha_squared) * (params.NdotL * params.NdotL)))) * (heaviside(params.HdotV) / (abs(params.NdotV) + sqrt(alpha_squared + ( .0 - alpha_squared) * (params.NdotV * params.NdotV))));

        float specular_brdf = params.lightIntensity * params.NdotL * visibility * distribution;

        vec3 diffuse_brdf = params.lightIntensity * params.NdotL * (1 / PI) * params.baseColor;

        vec3 conductor_fresnel = specular_brdf * ( params.baseColor + ( 1.0 - params.baseColor) * pow( 1.0 - abs( params.HdotV ), 5 ) );

        float f0 = 0.04; // pow( ( 1 - ior ) / ( 1 + ior ), 2 )
        float fr = f0 + ( 1 - f0 ) * pow(1 - abs( params.HdotV ), 5 );
        vec3 fresnel_mix = mix( diffuse_brdf, vec3( specular_brdf ), fr );

        material_colour = mix( fresnel_mix, conductor_fresnel, params.metallic );        
    }

    outColor = vec4(material_colour, 1.0);
}