#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormals;
layout(location = 3) in vec4 eye;
layout(location = 4) in vec4 light;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 emissiveContribution = vec3(1) * 0.1;
    
    // ambientContribution
    vec3 ambientContribution = vec3(1.0);
    
    // diffuseContribution
    vec3 diffuseContribution = vec3(1.0);
    
    // specularContribution
    vec3 specularContribution = vec3(1.0);

    // global light source
    vec3 lightDirection = normalize(vec3(light) - fragPosition);

    vec3 normals = fragNormals;
    
    // diffuse multiplier based on normals
    float diffuse = max(dot(normals, lightDirection), 0);

    // specular multiplier based on normals and eye direction
    vec3 eyeDirection = normalize(vec3(eye) - fragPosition);
    vec3 halfDirection = normalize(lightDirection + vec3(eye));
    float specular = pow(max(dot(normals, halfDirection), 0), 1);
        
    outColor = vec4(
        (emissiveContribution + ambientContribution + vec3(diffuse) + vec3(specular) * specularContribution.rgb) * 
            diffuseContribution.rgb, 1);
}
