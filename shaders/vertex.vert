#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
//layout (location = 2) in vec3 vTangent;
layout (location = 2) in vec2 vUV;
//layout (location = 4) in vec2 vColor;

// layout (location = 5) in vec4 m0;
// layout (location = 6) in vec4 m1;
// layout (location = 7) in vec4 m2;
// layout (location = 8) in vec4 m3;

layout (location = 0) out vec3 outPosition;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV;

layout(set = 0, binding = 0) uniform CameraBuffer
{
	mat4 view;
	mat4 projection;
	mat4 view_projection;
} camera;

layout(push_constant) uniform PushConstants
{
	mat4 transform;
} push_constants;

void main()
{
	mat4 transform = camera.view_projection * push_constants.transform;

	gl_Position = transform * vec4(vPosition, 1.0f);	
	outNormal = normalize(mat3(camera.view_projection) * vNormal);
	outPosition = vPosition;
	outUV = vUV;
}
