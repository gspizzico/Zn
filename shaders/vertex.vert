#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 uv;

layout (location = 0) out vec4 vPosition;
layout (location = 1) out vec3 vNormal;
layout (location = 2) out vec3 vTangent;
layout (location = 3) out vec2 vUV;

layout(set = 0, binding = 0) uniform CameraBuffer
{
	vec4 position;
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

	gl_Position = transform * vec4(position, 1.0f);	
	vPosition = push_constants.transform * vec4(position, 1.0);
	vNormal = mat3(camera.view_projection) * normal;
	vTangent = tangent;
	vUV = uv;
}
