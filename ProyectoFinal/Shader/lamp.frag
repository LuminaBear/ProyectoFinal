#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture_diffuse1;
uniform vec3 viewPos;

// Estructura de Luz Direccional (La que viene de la pared izquierda)
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// Estructura de Luz Puntual (Las del techo)
struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define NR_POINT_LIGHTS 6
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform DirLight dirLight;

// Función para calcular la luz ambiental direccional
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    
    // Sombreado difuso
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Sombreado especular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    
    // Resultados
    vec3 texColor = vec3(texture(texture_diffuse1, TexCoord));
    vec3 ambient = light.ambient * texColor;
    vec3 diffuse = light.diffuse * diff * texColor;
    vec3 specular = light.specular * spec * vec3(0.2);
    
    return (ambient + diffuse + specular);
}

// Función para calcular los focos del techo
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 texColor = vec3(texture(texture_diffuse1, TexCoord));
    vec3 ambient = light.ambient * texColor;
    vec3 diffuse = light.diffuse * diff * texColor;
    vec3 specular = light.specular * spec * vec3(0.2);

    return (ambient + diffuse + specular) * attenuation;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // 1. Sumamos primero la luz direccional (Ambiental)
    vec3 result = CalcDirLight(dirLight, norm, viewDir);
    
    // 2. Sumamos encima la luz de los 6 focos
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);

    FragColor = vec4(result, 1.0);
}