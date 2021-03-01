#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragNormals;
layout(location = 4) in vec4 eye;
layout(location = 5) in vec4 light;

layout(binding = 1) uniform sampler2D samplers[25];

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
    float ambientLightStrength = 1;
    vec4 ambientContribution = ambientLightColor * ambientLightStrength;
    
    // diffuseContribution
    vec4 diffuseLightColor = vec4(1.0);
    float diffuseLightStrength = 1.0;
    vec4 diffuseContribution = diffuseLightColor * diffuseLightStrength;    
    
    // specularContribution
    vec4 specularLightColor = vec4(1.0);
    float specularLightStrength = 1;
    vec4 specularContribution = specularLightColor * specularLightStrength;

    // global light source
    vec3 lightDirection = normalize(vec3(light) - fragPosition);

    // normals adjustment if normal texture is present
    vec3 normals = fragNormals;
    if (modelAttributes.normalTexture != -1) {
        normals = texture(samplers[modelAttributes.normalTexture], fragTexCoord).rgb;
        normals = normalize(normals * 2.0 - 1.0);
    }
    
    // diffuse multiplier based on normals
    float diffuse = max(dot(lightDirection, normals), 0.1);

    // specular multiplier based on normals and eye direction
    vec3 eyeDirection = normalize(vec3(eye) - fragPosition);
    vec3 halfDirection = normalize(lightDirection + vec3(eye));
    float shininess = 1;
    float specular = pow(max(dot(normals, halfDirection), 0.1), shininess);
    
    if (hasTextures) {
        // ambience
        if (modelAttributes.ambientTexture != -1) {
            ambientContribution *= texture(samplers[modelAttributes.ambientTexture], fragTexCoord);
        }
        
        // diffuse
        diffuseContribution = normalize(diffuseContribution * diffuse);
        if (modelAttributes.diffuseTexture != -1) {
            diffuseContribution *= texture(samplers[modelAttributes.diffuseTexture], fragTexCoord);
        }
        
        // sepcular
        specularContribution = normalize(specularContribution * specular);
        if (modelAttributes.specularTexture != -1) {
            specularContribution *= texture(samplers[modelAttributes.specularTexture], fragTexCoord);
        }        
    } else {
        ambientContribution *= vec4(fragColor, 1.0);
        diffuseContribution *= vec4(normalize(fragColor * diffuse),1.0);
        specularContribution *= vec4(normalize(fragColor * specular), 1.0);
    }
    
    outColor = mix(mix(ambientContribution, specularContribution, 0.85), diffuseContribution, 0.95);
}
