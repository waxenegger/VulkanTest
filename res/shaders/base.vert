#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 camera;
    vec4 sun;
} modelUniforms;

struct MeshProperties {
    int ambientTexture;
    int diffuseTexture;
    int specularTexture;
    int normalTexture;
    vec3 ambientColor;
    float emissiveFactor;
    vec3 diffuseColor;
    float opacity;
    vec3 specularColor;
    float shininess;
};

layout(push_constant) uniform PushConstants {
    mat4 matrix;
} modelProperties;

layout(std430, binding = 1) readonly buffer SSBO {
    MeshProperties props[];
} meshPropertiesSSBO;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormals;
layout(location = 3) out vec4 eye;
layout(location = 4) out vec4 light;
layout(location = 5) flat out MeshProperties meshProperties;

void main() {
    MeshProperties meshProps = meshPropertiesSSBO.props[gl_InstanceIndex];

    vec4 pos = modelProperties.matrix * vec4(inPosition, 1.0);

    gl_Position = modelUniforms.proj * modelUniforms.view * pos;
    fragPosition = vec3(pos);
    fragTexCoord = inUV;
    
    mat3 invertTransposeModel = mat3(transpose(inverse(modelProperties.matrix)));
    
    fragNormals = normalize(invertTransposeModel * inNormal);
    eye = modelUniforms.camera;
    light = modelUniforms.sun;
    meshProperties = meshProps;

    if (meshProps.normalTexture != -1) {
        vec3 T = normalize(invertTransposeModel * inTangent);
        vec3 N = normalize(invertTransposeModel * inNormal);
        vec3 B = normalize(invertTransposeModel * inBitangent);

        T = normalize(T - dot(T, N) * N);
        if (dot(cross(N, T), B) < 0.0f) T *= -1.0f;
        mat3 TBN = transpose(mat3(T, cross(N, T), N));

        pos = vec4(TBN * vec3(pos), 1.0);        
        eye = vec4(TBN * vec3(eye), 1.0);
        light = vec4(TBN * vec3(light), 1.0);	
    }
}
