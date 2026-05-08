#include <iostream>
#include <cmath>
#include <vector>
#include <string>

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Other Libs
#include "stb_image.h"

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Other includes
#include "Shader.h"
#include "Camera.h"

// Function prototypes
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
GLuint loadTexture(const char* path);

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, -0.5f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;

// Deltatime
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// Posiciones exactas de las 6 luces (En el centro de cada sección del techo)
glm::vec3 pointLightPositions[] = {
	glm::vec3(0.0f, 2.4f, -3.4f),
	glm::vec3(0.0f, 2.4f, -10.2f),
	glm::vec3(0.0f, 2.4f, -17.0f),
	glm::vec3(0.0f, 2.4f, -23.8f),
	glm::vec3(0.0f, 2.4f, -30.6f),
	glm::vec3(0.0f, 2.4f, -37.4f)
};

int main()
{
	// Init GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Túnel Iluminado", nullptr, nullptr);

	if (nullptr == window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewExperimental = GL_TRUE;
	if (GLEW_OK != glewInit())
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return EXIT_FAILURE;
	}

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glEnable(GL_DEPTH_TEST);

	Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");

	// Vértices 
	GLfloat vertices[] =
	{
		// Positions             // Normals (Hacia dónde apunta)  // Texture Coords

		// --- SUELO (Y = 1.0) ---
		-3.0f, -2.6f,  0.0f,     0.0f, 1.0f, 0.0f,     0.0f, 0.0f,
		 3.0f, -2.6f,  0.0f,	 0.0f, 1.0f, 0.0f,     1.0f, 0.0f,
		 3.0f, -2.6f, -6.8f,     0.0f, 1.0f, 0.0f,	   1.0f, 1.0f,
		-3.0f, -2.6f, -6.8f,     0.0f, 1.0f, 0.0f,     0.0f, 1.0f,

		// --- TECHO (Y = -1.0) ---
		-3.0f,  2.6f,  0.0f,     0.0f, -1.0f, 0.0f,    0.0f, 0.0f,
		 3.0f,  2.6f,  0.0f,	 0.0f, -1.0f, 0.0f,    1.0f, 0.0f,
		 3.0f,  2.6f, -6.8f,     0.0f, -1.0f, 0.0f,	   1.0f, 1.0f,
		-3.0f,  2.6f, -6.8f,     0.0f, -1.0f, 0.0f,    0.0f, 1.0f,

		// --- PARED IZQUIERDA (X = 1.0) ---
		-3.0f, -2.6f,  0.0f,     1.0f, 0.0f, 0.0f,     0.0f, 0.0f,
		-3.0f,  2.6f,  0.0f,	 1.0f, 0.0f, 0.0f,     0.0f, 1.0f,
		-3.0f,  2.6f, -6.8f,     1.0f, 0.0f, 0.0f,	   1.0f, 1.0f,
		-3.0f, -2.6f, -6.8f,     1.0f, 0.0f, 0.0f,     1.0f, 0.0f,

		// --- PARED DERECHA (X = -1.0) ---
		 3.0f, -2.6f,  0.0f,    -1.0f, 0.0f, 0.0f,     1.0f, 0.0f,
		 3.0f,  2.6f,  0.0f,	-1.0f, 0.0f, 0.0f,     1.0f, 1.0f,
		 3.0f,  2.6f, -6.8f,    -1.0f, 0.0f, 0.0f,	   0.0f, 1.0f,
		 3.0f, -2.6f, -6.8f,    -1.0f, 0.0f, 0.0f,     0.0f, 0.0f,

		 // --- PARED FINAL (Apunta hacia el túnel: Z = 1.0) ---
		 -3.0f, -2.6f,  0.0f,    0.0f, 0.0f, 1.0f,     1.0f, 0.0f,
		  3.0f, -2.6f,  0.0f,    0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
		  3.0f,  2.6f,  0.0f,    0.0f, 0.0f, 1.0f,     0.0f, 1.0f,
		 -3.0f,  2.6f,  0.0f,    0.0f, 0.0f, 1.0f,     1.0f, 1.0f,

		 // --- PARED INICIO (Apunta hacia adentro del túnel: Z = -1.0) ---
		 -3.0f, -2.6f,  0.0f,    0.0f, 0.0f, -1.0f,    1.0f, 0.0f,
		  3.0f, -2.6f,  0.0f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,
		  3.0f,  2.6f,  0.0f,    0.0f, 0.0f, -1.0f,    0.0f, 1.0f,
		 -3.0f,  2.6f,  0.0f,    0.0f, 0.0f, -1.0f,    1.0f, 1.0f
	};

	GLuint indices[] =
	{
		0,  1,  2,      2,  3,  0,  // Suelo
		4,  5,  6,      6,  7,  4,  // Techo
		8,  9, 10,     10, 11,  8,  // Pared Izquierda
	   12, 13, 14,     14, 15, 12,  // Pared Derecha
	   16, 17, 18,     18, 19, 16,  // Pared Final (Offset 24)
	   20, 21, 22,     22, 23, 20   // Pared Inicio (Offset 30)
	};

	GLuint VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	// Carga de Texturas
	GLuint texFloor = loadTexture("images/piso.jpg");
	GLuint texCeiling = loadTexture("images/techo.jpg");
	GLuint texFrontWall = loadTexture("images/pared_inicio.jpg");
	GLuint texBackWall = loadTexture("images/pared_final.jpg");

	GLuint texWallLeft[6];
	GLuint texWallRight[6];

	for (int i = 0; i < 6; i++) {
		std::string leftPath = "images/wall_left_" + std::to_string(i) + ".jpg";
		std::string rightPath = "images/wall_right_" + std::to_string(i) + ".jpg";
		texWallLeft[i] = loadTexture(leftPath.c_str());
		texWallRight[i] = loadTexture(rightPath.c_str());
	}

	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		DoMovement();

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lampShader.Use();

		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 100.0f);

		GLint modelLoc = glGetUniformLocation(lampShader.Program, "model");
		glUniformMatrix4fv(glGetUniformLocation(lampShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(lampShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		// --- CONFIGURACIÓN DE ILUMINACIÓN ---
		glm::vec3 camPos = camera.GetPosition();
		glUniform3f(glGetUniformLocation(lampShader.Program, "viewPos"), camPos.x, camPos.y, camPos.z);

		// 1. Luz Direccional (Ambiental desde la pared izquierda)
		// 1.0f en X significa que la luz viaja hacia la derecha.
		glUniform3f(glGetUniformLocation(lampShader.Program, "dirLight.direction"), 1.0f, -0.2f, 0.0f);
		glUniform3f(glGetUniformLocation(lampShader.Program, "dirLight.ambient"), 0.15f, 0.15f, 0.15f); // Luz base en todo el túnel
		glUniform3f(glGetUniformLocation(lampShader.Program, "dirLight.diffuse"), 0.35f, 0.35f, 0.35f); // Resalta la pared derecha
		glUniform3f(glGetUniformLocation(lampShader.Program, "dirLight.specular"), 0.1f, 0.1f, 0.1f);

		// 2. Configuramos las 6 luces puntuales del techo
		for (int i = 0; i < 6; i++)
		{
			std::string number = std::to_string(i);
			glUniform3f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].position").c_str()), pointLightPositions[i].x, pointLightPositions[i].y, pointLightPositions[i].z);
			glUniform3f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].ambient").c_str()), 0.05f, 0.05f, 0.05f);
			glUniform3f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].diffuse").c_str()), 0.8f, 0.8f, 0.8f);
			glUniform3f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].specular").c_str()), 1.0f, 1.0f, 1.0f);
			glUniform1f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].constant").c_str()), 1.0f);
			glUniform1f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].linear").c_str()), 0.09f);
			glUniform1f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].quadratic").c_str()), 0.032f);
		}
		// ------------------------------------

		glBindVertexArray(VAO);
		glActiveTexture(GL_TEXTURE0);

		for (int i = 0; i < 6; i++)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(0.0f, 0.0f, -6.8f * i));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

			glBindTexture(GL_TEXTURE_2D, texFloor);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

			glBindTexture(GL_TEXTURE_2D, texCeiling);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(GLuint)));

			glBindTexture(GL_TEXTURE_2D, texWallLeft[i]);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(12 * sizeof(GLuint)));

			glBindTexture(GL_TEXTURE_2D, texWallRight[i]);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(18 * sizeof(GLuint)));
		}

		// Renderizamos pared INICIO (Ahora toma los índices a partir del offset 30)
		glm::mat4 modelFront = glm::mat4(1.0f);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelFront));
		glBindTexture(GL_TEXTURE_2D, texFrontWall);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(30 * sizeof(GLuint)));

		// Renderizamos pared FINAL (Toma los índices a partir del offset 24)
		glm::mat4 modelBack = glm::mat4(1.0f);
		modelBack = glm::translate(modelBack, glm::vec3(0.0f, 0.0f, -40.8f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelBack));
		glBindTexture(GL_TEXTURE_2D, texBackWall);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(24 * sizeof(GLuint)));

		glBindVertexArray(0);
		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	glfwTerminate();
	return 0;
}

// Función auxiliar
GLuint loadTexture(const char* path)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);

	unsigned char* data = stbi_load(path, &width, &height, &nrChannels, STBI_rgb_alpha);

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else
	{
		std::cout << "¡ALERTA! No se pudo cargar: " << path << std::endl;
	}
	stbi_image_free(data);

	return textureID;
}

void DoMovement()
{
	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]) camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]) camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT]) camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) camera.ProcessKeyboard(RIGHT, deltaTime);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS) keys[key] = true;
		else if (action == GLFW_RELEASE) keys[key] = false;
	}
}

void MouseCallback(GLFWwindow* window, double xPos, double yPos)
{
	if (firstMouse)
	{
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}

	GLfloat xOffset = xPos - lastX;
	GLfloat yOffset = lastY - yPos;

	lastX = xPos;
	lastY = yPos;

	camera.ProcessMouseMovement(xOffset, yOffset);
}