#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int outNormal;

layout(location = 0) out vec4 outColor;

vec3 ambient = vec3(.5);
vec3 lightDirection = vec3(0.8, 1, 0.7);

vec3 normals[6] = {
    vec3 (-1, 0, 0),
    vec3 (1, 0, 0),
    vec3 (0, -1, 0),
    vec3 (0, 1, 0),
    vec3 (0, 0, -1),
    vec3 (0, 0, 1)
};

void main() {
    vec3 lightDir = normalize(-lightDirection);

	float diff = max(dot(normals[outNormal], lightDir), 0.0);
	vec3 diffuse = diff * vec3(1);

    vec4 result = vec4(ambient + diffuse, 1.0);

    outColor = texture(texSampler, fragTexCoord) * result;

    if (outColor.a <= 0) 
        discard;
}