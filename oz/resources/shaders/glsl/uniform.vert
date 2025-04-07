#version 450

layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
} mvp;

layout(set = 1, binding = 0) uniform Count {
    uint count;
} count;

layout(set = 1, binding = 1) uniform ColorArray {
    vec4 color[4];
} colorArray;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in uint index;

layout(location = 0) out vec3 fragColor;

void main() {

    int idx = gl_VertexIndex;
    gl_Position = mvp.proj * mvp.view * mvp.model * vec4(inPosition, 0.0, 1.0);
    fragColor = colorArray.color[idx].rgb;
}