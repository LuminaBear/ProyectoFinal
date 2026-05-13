#include <iostream>
#include <cmath>
#include <vector>
#include <string>

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Other includes
#include "Shader.h"
#include "Camera.h"
#include "Model.h"       // Importamos la clase Model
#include "SOIL2/SOIL2.h" // Usamos SOIL2 para TODAS las texturas del proyecto

// Function prototypes
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();

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

// --- VARIABLES DE ANIMACIÓN ESPACIAL DEL PERSONAJE ---
GLfloat personaZ = -2.0f;
bool isWalking = false;
GLfloat walkSpeed = 2.5f; 
GLfloat bobbingY = 0.0f;  // Variable para guardar el rebote vertical
// -----------------------------------------------------

// --- VARIABLE: DESPLAZAMIENTO DE LUCES Y PERSONAJE ---
glm::vec3 offsetTunel = glm::vec3(3.5f, 2.6f, 0.0f);
// -----------------------------------------------------

// Posiciones exactas de las 6 luces
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

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Túnel y Personaje", nullptr, nullptr);

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

	// ==============================================================
	// --- Carga de Modelos 3D ---
	Model personaWalking((GLchar*)"Models/n.fbx");

	Model m_paredes((GLchar*)"Models/paredes.obj");
	Model m_bancas((GLchar*)"Models/bancas.obj");
	Model m_coladeras((GLchar*)"Models/coladeras.obj");
	Model m_botebasura((GLchar*)"Models/botebasura.obj");
	Model m_buzon((GLchar*)"Models/buzon.obj");
	Model m_conexiones((GLchar*)"Models/conexiones.obj");
	Model m_sillas((GLchar*)"Models/sillas.obj");
	Model m_mesas((GLchar*)"Models/mesas.obj");
	Model m_paredstand((GLchar*)"Models/paredstand.obj");
	Model m_cilindrostand((GLchar*)"Models/cilindrostand.obj");
	Model m_abajostand((GLchar*)"Models/abajostand.obj");
	Model m_arribastand((GLchar*)"Models/arribastand.obj");
	// ==============================================================

	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		DoMovement();

		// --- LÓGICA DE TRASLACIÓN CON REBOTE (BOBBING) ---
		if (isWalking)
		{
			personaZ -= walkSpeed * deltaTime;

			// Función matemática para simular el paso:
			bobbingY = sin(glfwGetTime() * 10.0f) * 0.08f;

			if (personaZ <= -38.0f) {
				personaZ = -38.0f;
				isWalking = false;
				bobbingY = 0.0f; // Asegurarnos de que asiente los pies al detenerse
			}
		}
		// -------------------------------------------------

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

		glUniform3f(glGetUniformLocation(lampShader.Program, "dirLight.direction"), 1.0f, -0.2f, 0.0f);
		glUniform3f(glGetUniformLocation(lampShader.Program, "dirLight.ambient"), 0.15f, 0.15f, 0.15f);
		glUniform3f(glGetUniformLocation(lampShader.Program, "dirLight.diffuse"), 0.35f, 0.35f, 0.35f);
		glUniform3f(glGetUniformLocation(lampShader.Program, "dirLight.specular"), 0.1f, 0.1f, 0.1f);

		for (int i = 0; i < 6; i++)
		{
			std::string number = std::to_string(i);
			glUniform3f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].position").c_str()),
				pointLightPositions[i].x + offsetTunel.x,
				pointLightPositions[i].y + offsetTunel.y,
				pointLightPositions[i].z + offsetTunel.z);

			glUniform3f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].ambient").c_str()), 0.05f, 0.05f, 0.05f);
			glUniform3f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].diffuse").c_str()), 0.8f, 0.8f, 0.8f);
			glUniform3f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].specular").c_str()), 1.0f, 1.0f, 1.0f);
			glUniform1f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].constant").c_str()), 1.0f);
			glUniform1f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].linear").c_str()), 0.09f);
			glUniform1f(glGetUniformLocation(lampShader.Program, ("pointLights[" + number + "].quadratic").c_str()), 0.032f);
		}

		// ==============================================================
		// --- RENDERIZAR PERSONAJE ---
		glm::mat4 modelPersona = glm::mat4(1.0f);
		modelPersona = glm::translate(modelPersona, offsetTunel);

		// Inyectamos el movimiento vertical sumando bobbingY al eje Y
		modelPersona = glm::translate(modelPersona, glm::vec3(0.0f, -2.6f + bobbingY, personaZ));

		modelPersona = glm::scale(modelPersona, glm::vec3(0.038f, 0.038f, 0.038f));
		modelPersona = glm::rotate(modelPersona, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelPersona = glm::rotate(modelPersona, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelPersona));
		personaWalking.Draw(lampShader);

		// ==============================================================
		// --- RENDERIZAR NUEVOS MODELOS (Mobiliario y Entorno) ---
		glm::mat4 tmpModel = glm::mat4(1.0f);
		tmpModel = glm::rotate(tmpModel, glm::radians(90.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		tmpModel = glm::scale(tmpModel, glm::vec3(0.645f, 0.645f, 0.645f));

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(tmpModel));

		m_paredes.Draw(lampShader);
		m_paredstand.Draw(lampShader);
		m_bancas.Draw(lampShader);
		m_coladeras.Draw(lampShader);
		m_botebasura.Draw(lampShader);
		m_buzon.Draw(lampShader);
		m_conexiones.Draw(lampShader);
		m_sillas.Draw(lampShader);
		m_mesas.Draw(lampShader);
		m_cilindrostand.Draw(lampShader);
		m_abajostand.Draw(lampShader);
		m_arribastand.Draw(lampShader);
		// ==============================================================

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

void DoMovement()
{
	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]) camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]) camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT]) camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) camera.ProcessKeyboard(RIGHT, deltaTime);

	if (keys[GLFW_KEY_C]) {
		isWalking = true;
	}
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