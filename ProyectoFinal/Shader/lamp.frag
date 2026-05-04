#version 330 core

// Entradas que vienen del Vertex Shader
in vec3 ourColor;
in vec2 TexCoord;

// Salida: el color final del píxel
out vec4 color;

// Nuestro "muestreador" de texturas (aquí llega la imagen)
uniform sampler2D texture1;

void main()
{
    // Tomamos el color exacto del píxel de la textura en la coordenada correspondiente
    color = texture(texture1, TexCoord);
    
    // NOTA: Si quisieras mezclar el color de los vértices con la textura, 
    // usarías esta línea en lugar de la anterior:
    // color = texture(texture1, TexCoord) * vec4(ourColor, 1.0);
}