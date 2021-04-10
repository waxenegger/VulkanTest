#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 camera;
    vec4 sun;
} modelUniforms;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragNormals;
layout(location = 4) out vec4 eye;
layout(location = 5) out vec4 light;

void main() {
    vec4 pos = vec4(inPosition, 1.0);

    gl_Position = modelUniforms.proj * modelUniforms.view * pos;
    fragPosition = vec3(pos);
    fragColor = inColor;
    fragTexCoord = inUV;
    
    fragNormals = normalize(inNormal);
    eye = modelUniforms.camera;
    light = modelUniforms.sun;
}
