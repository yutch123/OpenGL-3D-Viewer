#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform vec3 objectColor;
uniform sampler2D texture_diffuse1;
uniform bool useTexture;
uniform vec3 viewPos;

uniform vec3 lightPos1;
uniform vec3 lightColor1;
uniform vec3 lightPos2;
uniform vec3 lightColor2;

uniform float lightIntensity1;
uniform float lightIntensity2;

void main()
{
    vec3 baseColor;
    if (useTexture)
        baseColor = texture(texture_diffuse1, TexCoords).rgb;
    else
        baseColor = objectColor;

    vec3 ambient = 0.25 * baseColor;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Первый источник света
    vec3 lightDir1 = normalize(lightPos1 - FragPos);
    float diff1 = max(dot(norm, lightDir1), 0.0);
    vec3 diffuse1 = diff1 * lightColor1 * lightIntensity1;

    vec3 reflectDir1 = reflect(-lightDir1, norm);
    float spec1 = pow(max(dot(viewDir, reflectDir1), 0.0), 16.0);

    float specularStrength = 0.2;
    vec3 specular1 = specularStrength * spec1 * lightColor1 * 0.5;

    // Второй источник света
    vec3 lightDir2 = normalize(lightPos2 - FragPos);
    float diff2 = max(dot(norm, lightDir2), 0.0);
    vec3 diffuse2 = diff2 * lightColor2 * lightIntensity2;

    // Итоговый цвет
    vec3 result = (ambient + diffuse1 + diffuse2) * baseColor + specular1;

    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));

    FragColor = vec4(result, 1.0);
}