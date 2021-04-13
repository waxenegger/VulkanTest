#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormals;
layout(location = 3) in vec4 eye;
layout(location = 4) in vec4 light;

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

layout(location = 5) flat in MeshProperties meshProperties;

layout(binding = 2) uniform sampler2D samplers[25];

layout(location = 0) out vec4 outColor;

void main() {
    bool hasTextures =
        meshProperties.ambientTexture != -1 || meshProperties.diffuseTexture != -1 ||
        meshProperties.specularTexture != -1 || meshProperties.normalTexture != -1;

    vec3 emissiveContribution = vec3(1) * meshProperties.emissiveFactor;
    
    // ambientContribution
    vec3 ambientContribution = meshProperties.ambientColor;
    
    // diffuseContribution
    vec3 diffuseContribution = meshProperties.diffuseColor;
    
    // specularContribution
    vec3 specularContribution = meshProperties.specularColor;

    // global light source
    vec3 lightDirection = normalize(vec3(light) - fragPosition);

    // normals adjustment if normal texture is present
    vec3 normals = fragNormals;
    if (meshProperties.normalTexture != -1) {
        normals = texture(samplers[meshProperties.normalTexture], fragTexCoord).rgb;
        normals = normalize(normals * 2.0 - 1.0);
    }
    
    // diffuse multiplier based on normals
    float diffuse = max(dot(normals, lightDirection), 0);

    // specular multiplier based on normals and eye direction
    vec3 eyeDirection = normalize(vec3(eye) - fragPosition);
    vec3 halfDirection = normalize(lightDirection + vec3(eye));
    float specular = pow(max(dot(normals, halfDirection), 0), meshProperties.shininess);
    
    if (hasTextures) {
        // ambience
        if (meshProperties.ambientTexture != -1) {
            ambientContribution = texture(samplers[meshProperties.ambientTexture], fragTexCoord).rgb;
        }
        
        // diffuse
        if (meshProperties.diffuseTexture != -1) {
            diffuseContribution = texture(samplers[meshProperties.diffuseTexture], fragTexCoord).rgb;
        }
        
        // sepcular
        if (meshProperties.specularTexture != -1) {
            specularContribution = texture(samplers[meshProperties.specularTexture], fragTexCoord).rgb;
        }
    }
    
    outColor = vec4(
        mix((emissiveContribution + ambientContribution + vec3(diffuse)) * diffuseContribution.rgb, 
        vec3(specular) * specularContribution.rgb, 0.40), meshProperties.opacity);
}
