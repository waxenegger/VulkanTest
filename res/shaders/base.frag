#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragNormals;
layout(location = 4) in vec4 eye;
layout(location = 5) in vec4 light;

struct ModelProperties {
    mat4 matrix;
    int ambientTexture;
    int diffuseTexture;
    int specularTexture;
    int normalTexture;
};

layout(location = 6) flat in ModelProperties modelProperties;

layout(binding = 2) uniform sampler2D samplers[25];

layout(location = 0) out vec4 outColor;

void main() {
    bool hasTextures =
        modelProperties.ambientTexture != -1 || modelProperties.diffuseTexture != -1 ||
        modelProperties.specularTexture != -1 || modelProperties.normalTexture != -1;

    // ambientContribution
    vec4 ambientLightColor = vec4(0.3);
    float ambientLightStrength = 1;
    vec4 ambientContribution = ambientLightColor * ambientLightStrength;
    
    // diffuseContribution
    vec4 diffuseLightColor = vec4(1.0);
    float diffuseLightStrength = 1.0;
    vec4 diffuseContribution = diffuseLightColor * diffuseLightStrength;    
    
    // specularContribution
    vec4 specularLightColor = vec4(1);
    float specularLightStrength = 0.5;
    vec4 specularContribution = specularLightColor * specularLightStrength;

    // global light source
    vec3 lightDirection = normalize(vec3(light) - fragPosition);

    // normals adjustment if normal texture is present
    vec3 normals = fragNormals;
    if (modelProperties.normalTexture != -1) {
        normals = texture(samplers[modelProperties.normalTexture], fragTexCoord).rgb;
        normals = normalize(normals * 2.0 - 1.0);
    }
    
    // diffuse multiplier based on normals
    float diffuse = max(dot(normals, lightDirection), 0.1);

    // specular multiplier based on normals and eye direction
    vec3 eyeDirection = normalize(vec3(eye) - fragPosition);
    vec3 halfDirection = normalize(lightDirection + vec3(eye));
    float shininess = 10;
    float specular = pow(max(dot(normals, halfDirection), 0.1), shininess);
    
    if (hasTextures) {
        // ambience
        if (modelProperties.ambientTexture != -1) {
            ambientContribution *= texture(samplers[modelProperties.ambientTexture], fragTexCoord);
        }
        
        // diffuse
        diffuseContribution = diffuseContribution * diffuse;
        if (modelProperties.diffuseTexture != -1) {
            diffuseContribution *= texture(samplers[modelProperties.diffuseTexture], fragTexCoord);
        }
        
        // sepcular
        specularContribution = specularContribution * specular;
        if (modelProperties.specularTexture != -1) {
            specularContribution *= texture(samplers[modelProperties.specularTexture], fragTexCoord);
        }
        
        outColor = mix(mix(ambientContribution, specularContribution, 0.5), diffuseContribution, 0.95);
    } else {
        ambientContribution *= vec4(fragColor, 1.0);
        diffuseContribution *= vec4(fragColor * diffuse,1.0) ;
        specularContribution *= vec4(fragColor * specular, 1.0);

        outColor = normalize(ambientContribution + diffuseContribution + specularContribution);
    }
}
