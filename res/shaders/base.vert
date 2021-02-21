#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} modelUniforms;

layout(push_constant) uniform PushConstants {
    mat4 matrix;
} modelAttributes;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = modelUniforms.proj * modelUniforms.view * modelAttributes.matrix * vec4(inPosition, 1.0);
    fragColor = inColor;
}
