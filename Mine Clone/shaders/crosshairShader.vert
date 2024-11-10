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

layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out int outNormal;

layout(push_constant) uniform PushConstantData {
    vec4 pos;
} chunkData;

void main() {
    float aspectRatio = float(ubo.height) / float(ubo.width);
    float scale = 0.1;
    // Adjust the x component by the aspect ratio
    vec2 adjustedPosition = vec2((inPosition.x - 0.5) * aspectRatio * scale, (inPosition.z - 0.5) * scale);

    gl_Position = vec4(adjustedPosition, 0.0, 1.0);

    fragTexCoord = inTexCoord;
    outNormal = inNormal;
}