#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform vec3 objectColor;
uniform sampler2D texture_diffuse1;
uniform bool useTexture;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

uniform sampler2D texture_specular1;
uniform sampler2D texture_specular2;

void main()
{
    vec3 baseColor;
    if (useTexture)
        baseColor = texture(texture_diffuse1, TexCoords).rgb;
    else
        baseColor = objectColor;

    vec3 ambient = 0.25 * baseColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 deffuse = 1.2 * diff * lightColor;
    vec3 lightPos2 = vec3(-3.0, 2.0, 2.0);
    vec3 lightColor2 = vec3(0.25);

    float specularStrength = 0.2;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = specularStrength * spec * lightColor * 0.5;
    vec3 lightDir2 = normalize(lightPos2 - FragPos);
    float diff2 = max(dot(norm, lightDir2), 0.0);
    vec3 diffuse2 = diff2 * lightColor2;

    vec3 result = (ambient + deffuse + diffuse2) * baseColor + specular;

    result = pow(result, vec3(1.0 / 2.2));

    FragColor = vec4(result, 1.0);
}
