#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 tangent;
layout (location = 3) in vec2 uv;

layout (location = 0) out vec3 vPosition;
layout (location = 1) out vec3 vNormal;
layout (location = 2) out vec3 vTangent;
layout (location = 3) out vec3 vBiTangent;
layout (location = 4) out vec2 vUV;

layout(std140, set = 0, binding = 0) uniform CameraBuffer
{
	vec4 position;
	mat4 view;
	mat4 projection;
	mat4 view_projection;
} camera;

layout(push_constant) uniform PushConstants
{
	mat4 model;
	mat4 model_inverse;
} push_constants;

void main()
{
	vec4 worldPosition = push_constants.model * vec4(position, 1.0f);
	gl_Position = camera.view_projection * worldPosition;
	vPosition = worldPosition.xyz / worldPosition.w;	
	vNormal = mat3(push_constants.model) * normal;

	vec4 thisTangent = tangent;
	if(tangent.xyz == vec3(0.0))
	{
		if (abs(normal.x) > abs(normal.y))
			vTangent = normalize(vec3(-normal.z, 0, normal.x));
		else
			vTangent = normalize(vec3(0, normal.z, -normal.y));

		thisTangent.w = 1.f;
	} else
	{
		vTangent = normalize(mat3(push_constants.model) * tangent.xyz);
	}

	vBiTangent = cross( vNormal, vTangent ) * thisTangent.w;
	vUV = uv;
}
