#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outPosition;
layout (location = 1) out vec3 outNormal;

layout(set = 0, binding = 0) uniform CameraBuffer
{
	mat4 view;
	mat4 projection;
	mat4 view_projection;
} camera;

//push constants block
layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
} PushConstants;

void main()
{
	mat4 transform = camera.view_projection * PushConstants.render_matrix;

	gl_Position = transform * vec4(vPosition, 1.0f);	
	outNormal = normalize(mat3(camera.view_projection) * vNormal);
	outPosition = vPosition;
}
