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
GLuint loadTexture(const char* path); // Nueva función para cargar texturas fácilmente

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, -0.5f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;

// Light attributes
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// Deltatime
GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame

// The MAIN function
int main()
{
	// Init GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Túnel con Múltiples Texturas", nullptr, nullptr);

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
		// Positions             // Colors             // Texture Coords
		// --- SUELO (y = -2.6) ---
		-3.0f, -2.6f,  0.0f,    1.0f, 1.0f, 1.0f,     0.0f, 0.0f,
		 3.0f, -2.6f,  0.0f,	1.0f, 1.0f, 1.0f,     6.0f, 0.0f,
		 3.0f, -2.6f, -6.8f,    1.0f, 1.0f, 1.0f,	  6.0f, 4.0f,
		-3.0f, -2.6f, -6.8f,    1.0f, 1.0f, 1.0f,     0.0f, 4.0f,

		// --- TECHO (y = 2.6) ---
		-3.0f,  2.6f,  0.0f,    1.0f, 1.0f, 1.0f,     0.0f, 0.0f,
		 3.0f,  2.6f,  0.0f,	1.0f, 1.0f, 1.0f,     6.0f, 0.0f,
		 3.0f,  2.6f, -6.8f,    1.0f, 1.0f, 1.0f,	  6.0f, 4.0f,
		-3.0f,  2.6f, -6.8f,    1.0f, 1.0f, 1.0f,     0.0f, 4.0f,

		// --- PARED IZQUIERDA (x = -3.0) ---
		-3.0f, -2.6f,  0.0f,    1.0f, 1.0f, 1.0f,     0.0f, 0.0f,
		-3.0f,  2.6f,  0.0f,	1.0f, 1.0f, 1.0f,     4.0f, 0.0f,
		-3.0f,  2.6f, -6.8f,    1.0f, 1.0f, 1.0f,	  4.0f, 4.0f,
		-3.0f, -2.6f, -6.8f,    1.0f, 1.0f, 1.0f,     0.0f, 4.0f,

		// --- PARED DERECHA (x = 3.0) ---
		 3.0f, -2.6f,  0.0f,    1.0f, 1.0f, 1.0f,     0.0f, 0.0f,
		 3.0f,  2.6f,  0.0f,	1.0f, 1.0f, 1.0f,     4.0f, 0.0f,
		 3.0f,  2.6f, -6.8f,    1.0f, 1.0f, 1.0f,	  4.0f, 4.0f,
		 3.0f, -2.6f, -6.8f,    1.0f, 1.0f, 1.0f,     0.0f, 4.0f,

		 // --- PARED DE CIERRE FRONTAL/TRASERA (Z = 0.0) ---
		 -3.0f, -2.6f,  0.0f,    1.0f, 1.0f, 1.0f,     0.0f, 0.0f,
		  3.0f, -2.6f,  0.0f,    1.0f, 1.0f, 1.0f,     6.0f, 0.0f,
		  3.0f,  2.6f,  0.0f,    1.0f, 1.0f, 1.0f,     6.0f, 5.2f,
		 -3.0f,  2.6f,  0.0f,    1.0f, 1.0f, 1.0f,     0.0f, 5.2f
	};

	// Índices 
	GLuint indices[] =
	{
		0,  1,  2,      2,  3,  0,  // Suelo (Offset 0)
		4,  5,  6,      6,  7,  4,  // Techo (Offset 6)
		8,  9, 10,     10, 11,  8,  // Pared Izquierda (Offset 12)
	   12, 13, 14,     14, 15, 12,  // Pared Derecha (Offset 18)
	   16, 17, 18,     18, 19, 16   // Pared de Cierre (Offset 24)
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

	// ==========================================
	// CARGA DE TEXTURAS
	// ==========================================
	GLuint texFloor = loadTexture("images/piso.png");
	GLuint texCeiling = loadTexture("images/techo.png");
	GLuint texFrontWall = loadTexture("images/pared_inicio.png");
	GLuint texBackWall = loadTexture("images/pared_final.png");

	// Arreglos para las texturas de las paredes laterales (5 secciones)
	GLuint texWallLeft[5];
	GLuint texWallRight[5];

	for (int i = 0; i < 5; i++) {
		std::string leftPath = "images/wall_left_" + std::to_string(i) + ".png";
		std::string rightPath = "images/wall_right_" + std::to_string(i) + ".png";
		texWallLeft[i] = loadTexture(leftPath.c_str());
		texWallRight[i] = loadTexture(rightPath.c_str());
	}
	// ==========================================

	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		DoMovement();

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lampShader.Use();

		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 100.0f);

		GLint modelLoc = glGetUniformLocation(lampShader.Program, "model");
		GLint viewLoc = glGetUniformLocation(lampShader.Program, "view");
		GLint projLoc = glGetUniformLocation(lampShader.Program, "projection");

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(VAO);
		glActiveTexture(GL_TEXTURE0);

		// 1. Renderizamos las 5 secciones del túnel separando las caras
		for (int i = 0; i < 5; i++)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(0.0f, 0.0f, -6.8f * i));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

			// Dibujar SUELO (Offset 0 índices)
			glBindTexture(GL_TEXTURE_2D, texFloor);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

			// Dibujar TECHO (Offset 6 índices * sizeof(GLuint))
			glBindTexture(GL_TEXTURE_2D, texCeiling);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(GLuint)));

			// Dibujar PARED IZQUIERDA (Offset 12 índices * sizeof(GLuint))
			glBindTexture(GL_TEXTURE_2D, texWallLeft[i]);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(12 * sizeof(GLuint)));

			// Dibujar PARED DERECHA (Offset 18 índices * sizeof(GLuint))
			glBindTexture(GL_TEXTURE_2D, texWallRight[i]);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(18 * sizeof(GLuint)));
		}

		// 2. Renderizamos la pared del INICIO (Z = 0.0)
		glm::mat4 modelFront = glm::mat4(1.0f);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelFront));
		glBindTexture(GL_TEXTURE_2D, texFrontWall);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(24 * sizeof(GLuint)));

		// 3. Renderizamos la pared del FINAL (Z = -34.0)
		glm::mat4 modelBack = glm::mat4(1.0f);
		modelBack = glm::translate(modelBack, glm::vec3(0.0f, 0.0f, -34.0f));
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

// ==========================================
// FUNCIÓN AUXILIAR PARA CARGAR TEXTURAS
// ==========================================
GLuint loadTexture(const char* path)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Parametros de envoltura y filtro
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

	if (data)
	{
		GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "No se encontro: " << path << ". Usando checker_Tex.png como respaldo." << std::endl;
		stbi_image_free(data); // Liberar memoria si falló

		// Cargar textura de respaldo
		data = stbi_load("images/checker_Tex.png", &width, &height, &nrChannels, 0);
		if (data) {
			GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}
	stbi_image_free(data);

	return textureID;
}

// Moves/alters the camera positions based on user input
void DoMovement()
{
	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]) camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]) camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT]) camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) camera.ProcessKeyboard(RIGHT, deltaTime);
}

// Is called whenever a key is pressed/released via GLFW
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