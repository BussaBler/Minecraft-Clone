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

const int waterFrames = 32;
const float animationTime = 8;
const int waterTexIdx = 8;

void main() {
    vec3 adjPos = inNormal == 5 ? vec3(inPosition.xy, inPosition.z - 0.1) : inPosition;
    vec3 worldPos = adjPos + chunkData.pos.xyz * 32;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(worldPos, 1.0);
    int frameIndex = int(floor(mod(chunkData.pos.w, animationTime) / animationTime * waterFrames));
    vec2 adjTex = inNormal == 5 ? vec2(inTexCoord.x + (16.0f / 1024.0f) * frameIndex, inTexCoord.y) : inTexCoord;
    fragTexCoord = adjTex;
    outNormal = inNormal;
}