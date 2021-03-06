#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragNormals;
layout(location = 4) in vec4 eye;
layout(location = 5) in vec4 light;

layout(location = 0) out vec4 outColor;

void main() {
    // global light source
    vec3 lightDirection = normalize(vec3(light) - fragPosition);

    vec3 normals = fragNormals;
    
    // diffuse multiplier based on normals
    float diffuse = max(dot(normals, lightDirection), 0.1);

    // specular multiplier based on normals and eye direction
    vec3 eyeDirection = normalize(vec3(eye) - fragPosition);
    vec3 halfDirection = normalize(lightDirection + vec3(eye));
    float specular = pow(max(dot(normals, halfDirection), 0.1), 1);
        
    outColor = vec4(mix(vec3(diffuse) * fragColor, vec3(specular) * fragColor, 0.4), 1);
}
