#version 430

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

//
uniform vec3 viewPos;
uniform vec4 ambient;

//
#define MAX_LIGHT_NUM 3
struct Light {
    int enabled;
    vec3 dir;
    vec4 color;
};
uniform Light lights[MAX_LIGHT_NUM];

void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 tint = colDiffuse * fragColor;

    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewPos - fragPosition);

    vec3 diffuse = vec3(0);
    vec3 specular = vec3(0);
    for (int i = 0; i < MAX_LIGHT_NUM; i++) {
        if (lights[i].enabled == 0) { continue; }

        vec3 light = -lights[i].dir;

        float NdotL = max(dot(normal, light), 0);
        diffuse += lights[i].color.rgb * NdotL;

        float shine = 16; // FIXME
        specular += pow(max(0, dot(viewD, reflect(-(light), normal))), shine);
    }

    finalColor = texelColor * (tint + vec4(specular, 1)) * vec4(diffuse , 1);
    finalColor += texelColor * ambient * tint;

    finalColor = pow(finalColor, vec4(1.0 / 2.2)); // wtf is this magic
}
