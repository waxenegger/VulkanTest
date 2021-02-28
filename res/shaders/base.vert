#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 camera;
    vec4 sun;
} modelUniforms;

layout(push_constant) uniform PushConstants {
    mat4 matrix;
    int ambientTexture;
    int diffuseTexture;
    int specularTexture;
    int normalTexture;
} modelAttributes;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragNormals;
layout(location = 4) out vec4 eye;
layout(location = 5) out vec4 light;

void main() {
    vec4 pos = modelAttributes.matrix * vec4(inPosition, 1.0);
    fragPosition = vec3(pos);
    fragColor = inColor;
    fragTexCoord = inUV;
    fragNormals = normalize(mat3(transpose(inverse(modelAttributes.matrix))) * inNormal);
    eye = modelUniforms.camera;
    light = modelUniforms.sun;

    gl_Position = modelUniforms.proj * modelUniforms.view * pos;
}
