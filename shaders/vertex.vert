#version 450

layout (location = 0) out vec3 outColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

const vec3 colors[3] = vec3[3](
	vec3(1.0f, 0.0f, 0.0f), //red
	vec3(0.0f, 1.0f, 0.0f), //green
	vec3(00.f, 0.0f, 1.0f)  //blue
);	

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	outColor = colors[gl_VertexIndex];
}