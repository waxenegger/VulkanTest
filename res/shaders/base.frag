#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D samplers[25];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 matrix;
    int ambientTexture;
    int diffuseTexture;
    int specularTexture;
    int normalTexture;
} modelAttributes;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
    
    bool hasTextures =
        modelAttributes.ambientTexture != -1 || modelAttributes.diffuseTexture != -1 ||
        modelAttributes.specularTexture != -1 || modelAttributes.normalTexture != -1;

    if (hasTextures) {
        if (modelAttributes.ambientTexture != -1) {
            outColor = normalize(outColor * texture(samplers[modelAttributes.ambientTexture], fragTexCoord));
        }

        if (modelAttributes.diffuseTexture != -1) {
            outColor = normalize(outColor * texture(samplers[modelAttributes.diffuseTexture], fragTexCoord));
        }

        if (modelAttributes.specularTexture != -1) {
            outColor = normalize(outColor * texture(samplers[modelAttributes.specularTexture], fragTexCoord));
        }
    }
}
