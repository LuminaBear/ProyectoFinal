#version 330 core

// Atributos que coinciden con los glVertexAttribPointer
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texCoord;

// Variables de salida
out vec3 ourColor;
out vec2 TexCoord;

// Matrices de transformación
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f);
    ourColor = color;
    
    // AQUÍ ESTÁ EL CAMBIO: Pasamos la coordenada original sin invertirla
    TexCoord = texCoord; 
}