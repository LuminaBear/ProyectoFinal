#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <ctime> // Para srand y time

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Otros includes
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "SOIL2/SOIL2.h"

// --- ESTRUCTURAS PARA ANIMACIÓN POR KEYFRAMES ---
struct Keyframe {
	float time;
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};

std::vector<Keyframe> standKeyframes = {
	{ 0.0f, glm::vec3(0.0f, 10.0f, 15.0f),  glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(0.05f) },
	{ 1.0f, glm::vec3(0.0f, 5.0f, 5.0f),   glm::vec3(0.0f, 180.0f, 0.0f),  glm::vec3(0.3f) },
	{ 2.0f, glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(0.645f) }
};

// --- ESTRUCTURAS PARA MULTITUD ---
enum Estado { IDLE, CAMINANDO, GIRANDO };

struct Personaje {
	glm::vec3 posicion;
	float rotacion;
	Estado estadoActual;
	float timerEstado;
	float timerLogica;
	float brazoIzq, brazoDer, piernaIzq, piernaDer;
	int tipoComportamiento;

	Personaje(glm::vec3 pos, float rot, int tipo = 0) {
		posicion = pos;
		rotacion = rot;
		estadoActual = IDLE;
		timerEstado = (float)(rand() % 10) / 10.0f;
		timerLogica = 0.0f;
		brazoIzq = brazoDer = piernaIzq = piernaDer = 0.0f;
		tipoComportamiento = tipo;
	}
};

bool playAnimation = false;
float animationTime = 0.0f;

// --- ALTURAS INDEPENDIENTES PARA CALIBRACIÓN ---
GLfloat alturaPersonaje = -2.6f;
GLfloat alturaMultitud = -1.6f;
// ----------------------------------------------

glm::vec3 lerp(glm::vec3 start, glm::vec3 end, float factor) {
	return start + factor * (end - start);
}

glm::mat4 getAnimationMatrix(const std::vector<Keyframe>& frames, float currentTime) {
	if (currentTime <= frames.front().time) {
		Keyframe first = frames.front();
		glm::mat4 model = glm::translate(glm::mat4(1.0f), first.position);
		model = glm::scale(model, first.scale);
		return model;
	}
	if (currentTime >= frames.back().time) {
		Keyframe last = frames.back();
		glm::mat4 model = glm::translate(glm::mat4(1.0f), last.position);
		model = glm::scale(model, last.scale);
		return model;
	}
	for (size_t i = 0; i < frames.size() - 1; i++) {
		if (currentTime >= frames[i].time && currentTime <= frames[i + 1].time) {
			float factor = (currentTime - frames[i].time) / (frames[i + 1].time - frames[i].time);
			glm::vec3 pos = lerp(frames[i].position, frames[i + 1].position, factor);
			glm::vec3 rot = lerp(frames[i].rotation, frames[i + 1].rotation, factor);
			glm::vec3 sca = lerp(frames[i].scale, frames[i + 1].scale, factor);
			glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
			model = glm::rotate(model, glm::radians(rot.y), glm::vec3(0, 1, 0));
			return glm::scale(model, sca);
		}
	}
	return glm::mat4(1.0f);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
void UpdateAnimation(Personaje& p, float deltaTime, bool keys[]);

const GLuint WIDTH = 800, HEIGHT = 600;
int SCREEN_WIDTH, SCREEN_HEIGHT;
Camera camera(glm::vec3(0.0f, 0.0f, -0.5f));
GLfloat lastX = WIDTH / 2.0, lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;
GLfloat deltaTime = 0.0f, lastFrame = 0.0f;
glm::vec3 offsetTunel = glm::vec3(3.5f, 2.6f, 0.0f);

glm::vec3 pointLightPositions[] = {
	glm::vec3(0.0f, 2.4f, -3.4f), glm::vec3(0.0f, 2.4f, -10.2f),
	glm::vec3(0.0f, 2.4f, -17.0f), glm::vec3(0.0f, 2.4f, -23.8f),
	glm::vec3(0.0f, 2.4f, -30.6f), glm::vec3(0.0f, 2.4f, -37.4f)
};

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Túnel y Exposicion", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewExperimental = GL_TRUE;
	glewInit();

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glEnable(GL_DEPTH_TEST);

	Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");

	// Modelos Originales
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

	// Modelos Multitud
	Model m_torso((GLchar*)"Models/torso.obj");
	Model m_arribaIzq((GLchar*)"Models/arribaizquierda.obj");
	Model m_hombroDer((GLchar*)"Models/hombroderecha.obj");
	Model m_manoDer((GLchar*)"Models/manoderecha.obj");
	Model m_manoIzq((GLchar*)"Models/manoizquierda.obj");
	Model m_pieDer((GLchar*)"Models/piederecha.obj");
	Model m_pieIzq((GLchar*)"Models/pieizquierda.obj");
	Model m_piernaDer((GLchar*)"Models/piernaderecha.obj");
	Model m_piernaIzq((GLchar*)"Models/piernaizquierda.obj");

	srand((unsigned int)time(NULL));
	std::vector<Personaje> multitud;

	// --- INICIALIZACIÓN DE LA MULTITUD EN CASCADA ---

	// 0: Grupo principal (Caminan hasta el final del túnel)
	multitud.push_back(Personaje(glm::vec3(-0.65f, 0.0f, 0.0f), 0.0f, 0));
	multitud.push_back(Personaje(glm::vec3(0.0f, 0.0f, 0.05f), 0.0f, 0));
	multitud.push_back(Personaje(glm::vec3(0.65f, 0.0f, 0.0f), 0.0f, 0));

	// 1: Detrás, se detienen cerca del inicio (Z=8)
	multitud.push_back(Personaje(glm::vec3(0.0f, 0.0f, -3.0f), 0.0f, 1));
	multitud.push_back(Personaje(glm::vec3(-0.5f, 0.0f, -4.0f), 0.0f, 1));

	// 2: Más atrás, se detienen a medio pasillo (Z=16)
	multitud.push_back(Personaje(glm::vec3(0.4f, 0.0f, -7.0f), 0.0f, 2));
	multitud.push_back(Personaje(glm::vec3(-0.3f, 0.0f, -8.0f), 0.0f, 2));

	// 5: Aún más atrás, se detienen lejos (Z=24)
	multitud.push_back(Personaje(glm::vec3(0.2f, 0.0f, -11.0f), 0.0f, 5));
	multitud.push_back(Personaje(glm::vec3(-0.6f, 0.0f, -12.0f), 0.0f, 5));

	// 6: Los últimos, se detienen casi al final (Z=30)
	multitud.push_back(Personaje(glm::vec3(0.0f, 0.0f, -15.0f), 0.0f, 6));

	while (!glfwWindowShouldClose(window)) {
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		DoMovement();

		if (playAnimation) animationTime += deltaTime;

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lampShader.Use();
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 100.0f);
		GLint modelLoc = glGetUniformLocation(lampShader.Program, "model");
		glUniformMatrix4fv(glGetUniformLocation(lampShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(lampShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		// Iluminación
		glm::vec3 camPos = camera.GetPosition();
		glUniform3f(glGetUniformLocation(lampShader.Program, "viewPos"), camPos.x, camPos.y, camPos.z);
		glUniform3f(glGetUniformLocation(lampShader.Program, "dirLight.direction"), 1.0f, -0.2f, 0.0f);
		glUniform3f(glGetUniformLocation(lampShader.Program, "dirLight.ambient"), 0.15f, 0.15f, 0.15f);
		glUniform3f(glGetUniformLocation(lampShader.Program, "dirLight.diffuse"), 0.35f, 0.35f, 0.35f);
		glUniform3f(glGetUniformLocation(lampShader.Program, "dirLight.specular"), 0.1f, 0.1f, 0.1f);

		for (int i = 0; i < 6; i++) {
			std::string n = std::to_string(i);
			glUniform3f(glGetUniformLocation(lampShader.Program, ("pointLights[" + n + "].position").c_str()), pointLightPositions[i].x + offsetTunel.x, pointLightPositions[i].y + offsetTunel.y, pointLightPositions[i].z + offsetTunel.z);
			glUniform3f(glGetUniformLocation(lampShader.Program, ("pointLights[" + n + "].ambient").c_str()), 0.05f, 0.05f, 0.05f);
			glUniform3f(glGetUniformLocation(lampShader.Program, ("pointLights[" + n + "].diffuse").c_str()), 0.8f, 0.8f, 0.8f);
			glUniform1f(glGetUniformLocation(lampShader.Program, ("pointLights[" + n + "].constant").c_str()), 1.0f);
			glUniform1f(glGetUniformLocation(lampShader.Program, ("pointLights[" + n + "].linear").c_str()), 0.09f);
			glUniform1f(glGetUniformLocation(lampShader.Program, ("pointLights[" + n + "].quadratic").c_str()), 0.032f);
		}


		// ==============================================================
		// --- RENDERIZAR EXPOSITORES
		float expositoresZ[] = { -9.95f, -17.78f, -26.55f, -33.22f };

		for (int i = 0; i < 4; i++) {
			glm::mat4 mExpositor = glm::mat4(1.0f);
			// Los ponemos del lado izquierdo (X = -1.5f) para que queden frente a la multitud
			mExpositor = glm::translate(mExpositor, offsetTunel + glm::vec3(-1.5f, alturaPersonaje, expositoresZ[i]));
			mExpositor = glm::scale(mExpositor, glm::vec3(0.025f)); // Misma escala

			// Rotamos 90 grados para que vean hacia la derecha (+X)
			mExpositor = glm::rotate(mExpositor, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			mExpositor = glm::rotate(mExpositor, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mExpositor));
			personaWalking.Draw(lampShader);
		}
		// ==============================================================


		// Estáticos
		glm::mat4 baseRot = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		glm::mat4 staticTransform = glm::scale(baseRot, glm::vec3(0.645f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(staticTransform));
		m_paredes.Draw(lampShader);
		m_bancas.Draw(lampShader);
		m_coladeras.Draw(lampShader);
		m_botebasura.Draw(lampShader);
		m_buzon.Draw(lampShader);
		m_conexiones.Draw(lampShader);
		m_sillas.Draw(lampShader);
		m_mesas.Draw(lampShader);

		// STAND DINÁMICO 
		if (!playAnimation) {
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(staticTransform));
			m_abajostand.Draw(lampShader);
			m_cilindrostand.Draw(lampShader);
			m_arribastand.Draw(lampShader);
			m_paredstand.Draw(lampShader);
		}
		else {
			// Ensamblaje 
			glm::mat4 mAbajo = getAnimationMatrix(standKeyframes, animationTime - 0.0f) * baseRot;
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mAbajo));
			m_abajostand.Draw(lampShader);

			glm::mat4 mCilindro = getAnimationMatrix(standKeyframes, animationTime - 0.5f) * baseRot;
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mCilindro));
			m_cilindrostand.Draw(lampShader);

			glm::mat4 mArriba = getAnimationMatrix(standKeyframes, animationTime - 1.0f) * baseRot;
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mArriba));
			m_arribastand.Draw(lampShader);

			glm::mat4 mPared = getAnimationMatrix(standKeyframes, animationTime - 1.5f) * baseRot;
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mPared));
			m_paredstand.Draw(lampShader);
		}

		// Renderizado de la Multitud
		for (size_t i = 0; i < multitud.size(); i++) {
			UpdateAnimation(multitud[i], deltaTime, keys);

			glm::mat4 baseMultitudTransform = glm::translate(glm::mat4(1.0f), offsetTunel + glm::vec3(0.0f, alturaMultitud, -2.0f));
			baseMultitudTransform = glm::rotate(baseMultitudTransform, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

			// Aplicar las transformaciones individuales
			glm::mat4 model = glm::translate(baseMultitudTransform, multitud[i].posicion);
			model = glm::rotate(model, glm::radians(multitud[i].rotacion), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 torsoBase = model;

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(torsoBase));
			m_torso.Draw(lampShader);

			// Render extremidades
			glm::mat4 m_manoIzqMat = torsoBase;
			glm::vec3 pivoteIzq = glm::vec3(0.5f, 1.0f, 0.0f);
			m_manoIzqMat = glm::translate(m_manoIzqMat, pivoteIzq);
			m_manoIzqMat = glm::rotate(m_manoIzqMat, glm::radians(multitud[i].brazoIzq), glm::vec3(1.0f, 0.0f, 0.0f));
			m_manoIzqMat = glm::translate(m_manoIzqMat, -pivoteIzq);
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m_manoIzqMat));
			m_arribaIzq.Draw(lampShader); m_manoIzq.Draw(lampShader);

			glm::mat4 m_manoDerMat = torsoBase;
			glm::vec3 pivoteDer = glm::vec3(-0.5f, 1.0f, 0.0f);
			m_manoDerMat = glm::translate(m_manoDerMat, pivoteDer);
			m_manoDerMat = glm::rotate(m_manoDerMat, glm::radians(multitud[i].brazoDer), glm::vec3(1.0f, 0.0f, 0.0f));
			m_manoDerMat = glm::translate(m_manoDerMat, -pivoteDer);
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m_manoDerMat));
			m_hombroDer.Draw(lampShader); m_manoDer.Draw(lampShader);

			glm::mat4 m_piernaIzqMat = torsoBase;
			m_piernaIzqMat = glm::rotate(m_piernaIzqMat, glm::radians(multitud[i].piernaIzq), glm::vec3(1.0f, 0.0f, 0.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m_piernaIzqMat));
			m_piernaIzq.Draw(lampShader); m_pieIzq.Draw(lampShader);

			glm::mat4 m_piernaDerMat = torsoBase;
			m_piernaDerMat = glm::rotate(m_piernaDerMat, glm::radians(multitud[i].piernaDer), glm::vec3(1.0f, 0.0f, 0.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m_piernaDerMat));
			m_piernaDer.Draw(lampShader); m_pieDer.Draw(lampShader);
		}

		glfwSwapBuffers(window);
	}
	glfwTerminate();
	return 0;
}

void DoMovement() {
	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]) camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]) camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT]) camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) camera.ProcessKeyboard(RIGHT, deltaTime);
	if (keys[GLFW_KEY_K]) { playAnimation = true; animationTime = 0.0f; }
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS) keys[key] = true;
		else if (action == GLFW_RELEASE) keys[key] = false;
	}
}

void MouseCallback(GLFWwindow* window, double xPos, double yPos) {
	if (firstMouse) { lastX = xPos; lastY = yPos; firstMouse = false; }
	camera.ProcessMouseMovement(xPos - lastX, lastY - yPos);
	lastX = xPos; lastY = yPos;
}

// --- LÓGICA DE ANIMACIÓN ---
void UpdateAnimation(Personaje& p, float deltaTime, bool keys[]) {
	if (p.estadoActual == IDLE && keys[GLFW_KEY_N]) {
		p.estadoActual = CAMINANDO;
	}

	if (p.estadoActual == CAMINANDO || p.estadoActual == GIRANDO) {
		p.timerEstado += deltaTime * 5.0f;
		float osc = sin(p.timerEstado);
		p.brazoIzq = osc * 15.0f; p.brazoDer = -osc * 15.0f;
		p.piernaIzq = -osc * 25.0f; p.piernaDer = osc * 25.0f;
	}
	else {
		p.brazoIzq = p.brazoDer = p.piernaIzq = p.piernaDer = 0.0f;
	}

	if (p.tipoComportamiento == 0) { // Grupo principal
		if (p.estadoActual == CAMINANDO) {
			if (p.posicion.z < 36.0f) p.posicion.z += 1.5f * deltaTime;
			else { p.posicion.z = 36.0f; p.estadoActual = IDLE; }
		}
	}
	// Generalización para los que se detienen a lo largo del pasillo (Tipos 1, 2, 5 y 6)
	else if (p.tipoComportamiento == 1 || p.tipoComportamiento == 2 || p.tipoComportamiento == 5 || p.tipoComportamiento == 6) {
		float limiteZ = 0.0f;
		if (p.tipoComportamiento == 1) limiteZ = 8.0f;
		else if (p.tipoComportamiento == 2) limiteZ = 16.0f;
		else if (p.tipoComportamiento == 5) limiteZ = 24.0f;
		else if (p.tipoComportamiento == 6) limiteZ = 30.0f;

		if (p.estadoActual == CAMINANDO) {
			if (p.posicion.z < limiteZ) p.posicion.z += 1.6f * deltaTime;
			else p.estadoActual = GIRANDO;
		}
		else if (p.estadoActual == GIRANDO) {
			if (p.rotacion < 90.0f) p.rotacion += 100.0f * deltaTime;
			else { p.rotacion = 90.0f; p.estadoActual = IDLE; }
		}
	}
}