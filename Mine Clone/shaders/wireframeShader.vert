#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    int width;
    int height;
} ubo;

layout(location = 0) in uvec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in int inNormal;

layout(push_constant) uniform PushConstantData {
    vec4 pos;
} chunkData;

void main() {
    vec3 worldPos = inPosition + chunkData.pos.xyz;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(worldPos, 1.0);
}