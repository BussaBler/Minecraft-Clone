#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in uvec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in int inNormal;

layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out int outNormal;

layout(push_constant) uniform PushConstantData {
    vec3 pos;
} chunkData;

void main() {
    vec3 worldPos = inPosition + chunkData.pos * 32;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(worldPos, 1.0);
    fragTexCoord = inTexCoord;
    outNormal = inNormal;
}