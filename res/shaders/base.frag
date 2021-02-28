#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D samplers[25];

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragNormals;
layout(location = 4) in vec4 eye;
layout(location = 5) in vec4 light;

layout(push_constant) uniform PushConstants {
    mat4 matrix;
    int ambientTexture;
    int diffuseTexture;
    int specularTexture;
    int normalTexture;
} modelAttributes;

layout(location = 0) out vec4 outColor;

void main() {
    bool hasTextures =
        modelAttributes.ambientTexture != -1 || modelAttributes.diffuseTexture != -1 ||
        modelAttributes.specularTexture != -1 || modelAttributes.normalTexture != -1;

    // ambientContribution
    vec4 ambientLightColor = vec4(1.0);
    float ambientLightStrength = 0.1;
    vec4 ambientContribution = ambientLightColor * ambientLightStrength;
    
    // diffuseContribution
    vec4 diffuseLightColor = vec4(1.0);
    float diffuseLightStrength = 0.9;
    vec4 diffuseContribution = diffuseLightColor * diffuseLightStrength;    
    
    // specularContribution
    vec4 specularLightColor = vec4(1.0);
    float specularLightStrength = 0.2;
    vec4 specularContribution = specularLightColor * specularLightStrength;

    // global light source
    vec3 lightDirection = normalize(vec3(light) - fragPosition);
    // diffuse multiplier based on normals
    float diffuse = clamp(dot(lightDirection, fragNormals), 0, 1);

    if (hasTextures) {
        // ambience
        if (modelAttributes.ambientTexture != -1) {
            ambientContribution *= texture(samplers[modelAttributes.ambientTexture], fragTexCoord);
        }
        
        // diffuse
        diffuseContribution = vec4(1.0) * diffuse;
        if (modelAttributes.diffuseTexture != -1) {
            diffuseContribution *= texture(samplers[modelAttributes.diffuseTexture], fragTexCoord);
        }
        
        // sepcular
        if (modelAttributes.specularTexture != -1) {
            specularContribution *= texture(samplers[modelAttributes.specularTexture], fragTexCoord);
        }        
    } else {
        vec4 baseColor = vec4(fragColor, 1.0);
        ambientContribution *= baseColor;
        diffuseContribution *= baseColor * diffuse;
        specularContribution *= baseColor;
    }
    
    outColor = mix(mix(ambientContribution, specularContribution, 0.75), diffuseContribution, 0.95);
}
