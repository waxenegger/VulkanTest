#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 6) in mat4 inModelMatrix;

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
};

layout(std430, binding = 1) readonly buffer SSBO {
    MeshProperties props[];
} meshPropertiesSSBO;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragNormals;
layout(location = 4) out vec4 eye;
layout(location = 5) out vec4 light;
layout(location = 6) out MeshProperties meshProperties;

void main() {
    MeshProperties meshProps = meshPropertiesSSBO.props[gl_BaseInstance];
 
    vec4 pos = inModelMatrix * vec4(inPosition, 1.0);
    fragPosition = vec3(pos);
    fragColor = inColor;
    fragTexCoord = inUV;
    
    mat3 invertTransposeModel = mat3(transpose(inverse(inModelMatrix)));
    
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
    
    gl_Position = modelUniforms.proj * modelUniforms.view * pos;
}
