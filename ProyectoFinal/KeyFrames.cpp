// Proyecto Final 
#include <vector>
#include <ctime>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SOIL2/SOIL2.h"
#include "Shader.h"
#include "Camera.h"
#include "Model.h"

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

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
void UpdateAnimation(Personaje& p, float deltaTime, bool keys[]);

const GLuint WIDTH = 1280, HEIGHT = 720;
Camera camera(glm::vec3(0.0f, 5.0f, 15.0f));
bool keys[1024];
GLfloat deltaTime = 0.0f, lastFrame = 0.0f;
GLfloat lastX = WIDTH / 2.0, lastY = HEIGHT / 2.0;
bool firstMouse = true;

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Proyecto Final", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glewExperimental = GL_TRUE;
    glewInit();
    glEnable(GL_DEPTH_TEST);

    Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");
    Model m_torso((char*)"Models/torso.obj");
    Model m_arribaIzq((char*)"Models/arribaizquierda.obj");
    Model m_hombroDer((char*)"Models/hombroderecha.obj");
    Model m_manoDer((char*)"Models/manoderecha.obj");
    Model m_manoIzq((char*)"Models/manoizquierda.obj");
    Model m_pieDer((char*)"Models/piederecha.obj");
    Model m_pieIzq((char*)"Models/pieizquierda.obj");
    Model m_piernaDer((char*)"Models/piernaderecha.obj");
    Model m_piernaIzq((char*)"Models/piernaizquierda.obj");
    Model m_piso((char*)"Models/piso.obj");

    srand((unsigned int)time(NULL));
    std::vector<Personaje> multitud;

    // 0: Grupo
    multitud.push_back(Personaje(glm::vec3(-0.65f, 0.0f, 0.0f), 0.0f, 0));
    multitud.push_back(Personaje(glm::vec3(0.0f, 0.0f, 0.05f), 0.0f, 0));
    multitud.push_back(Personaje(glm::vec3(0.65f, 0.0f, 0.0f), 0.0f, 0));
    multitud.push_back(Personaje(glm::vec3(-0.35f, 0.0f, -0.8f), 0.0f, 0));
    multitud.push_back(Personaje(glm::vec3(0.35f, 0.0f, -0.8f), 0.0f, 0));
    // 1: Solitaria
    multitud.push_back(Personaje(glm::vec3(0.0f, 0.0f, -4.0f), 0.0f, 1));
    // 2: Atrás
    multitud.push_back(Personaje(glm::vec3(-0.3f, 0.0f, -6.0f), 0.0f, 2));
    multitud.push_back(Personaje(glm::vec3(0.3f, 0.0f, -6.0f), 0.0f, 2));

    // 3: Persona 9 (Viendo al revés en Z=10)
    multitud.push_back(Personaje(glm::vec3(-2.0f, 0.0f, 10.0f), 180.0f, 3));

    // 4: Persona 10 (Igual que la 9, pero camina más al inicio)
    multitud.push_back(Personaje(glm::vec3(-4.0f, 0.0f, 10.0f), 180.0f, 4));

    while (!glfwWindowShouldClose(window)) {
        GLfloat currentFrame = (GLfloat)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        glfwPollEvents();
        DoMovement();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        lightingShader.Use();
        glm::mat4 projection = glm::perspective(camera.GetZoom(), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"), 0.3f, 0.3f, 0.3f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"), 0.6f, 0.6f, 0.6f);
        GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");

        glm::mat4 modelPiso = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelPiso));
        m_piso.Draw(lightingShader);

        for (size_t i = 0; i < multitud.size(); i++) {
            UpdateAnimation(multitud[i], deltaTime, keys);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), multitud[i].posicion);
            model = glm::rotate(model, glm::radians(multitud[i].rotacion), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 torsoBase = model;

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(torsoBase));
            m_torso.Draw(lightingShader);

            // Render extremidades
            glm::mat4 m_manoIzqMat = torsoBase;
            glm::vec3 pivoteIzq = glm::vec3(0.5f, 1.0f, 0.0f);
            m_manoIzqMat = glm::translate(m_manoIzqMat, pivoteIzq);
            m_manoIzqMat = glm::rotate(m_manoIzqMat, glm::radians(multitud[i].brazoIzq), glm::vec3(1.0f, 0.0f, 0.0f));
            m_manoIzqMat = glm::translate(m_manoIzqMat, -pivoteIzq);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m_manoIzqMat));
            m_arribaIzq.Draw(lightingShader); m_manoIzq.Draw(lightingShader);

            glm::mat4 m_manoDerMat = torsoBase;
            glm::vec3 pivoteDer = glm::vec3(-0.5f, 1.0f, 0.0f);
            m_manoDerMat = glm::translate(m_manoDerMat, pivoteDer);
            m_manoDerMat = glm::rotate(m_manoDerMat, glm::radians(multitud[i].brazoDer), glm::vec3(1.0f, 0.0f, 0.0f));
            m_manoDerMat = glm::translate(m_manoDerMat, -pivoteDer);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m_manoDerMat));
            m_hombroDer.Draw(lightingShader); m_manoDer.Draw(lightingShader);

            glm::mat4 m_piernaIzqMat = torsoBase;
            m_piernaIzqMat = glm::rotate(m_piernaIzqMat, glm::radians(multitud[i].piernaIzq), glm::vec3(1.0f, 0.0f, 0.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m_piernaIzqMat));
            m_piernaIzq.Draw(lightingShader); m_pieIzq.Draw(lightingShader);

            glm::mat4 m_piernaDerMat = torsoBase;
            m_piernaDerMat = glm::rotate(m_piernaDerMat, glm::radians(multitud[i].piernaDer), glm::vec3(1.0f, 0.0f, 0.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m_piernaDerMat));
            m_piernaDer.Draw(lightingShader); m_pieDer.Draw(lightingShader);
        }
        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}

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

    if (p.tipoComportamiento == 0) {
        if (p.estadoActual == CAMINANDO) {
            if (p.posicion.z < 10.0f) p.posicion.z += 1.5f * deltaTime;
            else { p.posicion.z = 10.0f; p.estadoActual = IDLE; }
        }
    }
    else if (p.tipoComportamiento == 1) {
        if (p.estadoActual == CAMINANDO) {
            if (p.posicion.z < 7.0f) p.posicion.z += 1.8f * deltaTime;
            else p.estadoActual = GIRANDO;
        }
        else if (p.estadoActual == GIRANDO) {
            if (p.rotacion < 90.0f) p.rotacion += 100.0f * deltaTime;
            else { p.rotacion = 90.0f; p.estadoActual = IDLE; }
        }
    }
    else if (p.tipoComportamiento == 2) {
        if (p.estadoActual == CAMINANDO) {
            p.timerLogica += deltaTime;
            p.posicion.z += 2.2f * deltaTime;
            if (p.timerLogica > 4.5f) p.estadoActual = GIRANDO;
        }
        else if (p.estadoActual == GIRANDO) {
            if (p.rotacion < 90.0f) p.rotacion += 120.0f * deltaTime;
            else { p.rotacion = 90.0f; p.estadoActual = IDLE; }
        }
    }
    else if (p.tipoComportamiento == 3) { // PERSONA 9
        if (p.estadoActual == CAMINANDO) {
            if (p.timerLogica < 6.7f) p.timerLogica += deltaTime;
            else {
                if (p.rotacion == 180.0f) {
                    if (p.posicion.z > 6.0f) p.posicion.z -= 1.5f * deltaTime;
                    else p.estadoActual = GIRANDO;
                }
                else if (p.rotacion == 90.0f) {
                    if (p.posicion.x < 5.0f) p.posicion.x += 1.5f * deltaTime;
                    else p.estadoActual = IDLE;
                }
            }
        }
        else if (p.estadoActual == GIRANDO) {
            if (p.rotacion > 90.0f) p.rotacion -= 100.0f * deltaTime;
            else { p.rotacion = 90.0f; p.estadoActual = CAMINANDO; }
        }
    }
    else if (p.tipoComportamiento == 4) { 
        if (p.estadoActual == CAMINANDO) {
            if (p.timerLogica < 5.0f) p.timerLogica += deltaTime; 
            else {
                if (p.rotacion == 180.0f) {

                    if (p.posicion.z > 4.0f) p.posicion.z -= 1.5f * deltaTime;
                    else p.estadoActual = GIRANDO;
                }
                else if (p.rotacion == 90.0f) {
                    if (p.posicion.x < 5.0f) p.posicion.x += 1.5f * deltaTime;
                    else p.estadoActual = IDLE;
                }
            }
        }
        else if (p.estadoActual == GIRANDO) {
            if (p.rotacion > 90.0f) p.rotacion -= 100.0f * deltaTime;
            else { p.rotacion = 90.0f; p.estadoActual = CAMINANDO; }
        }
    }
}

void DoMovement() {
    if (keys[GLFW_KEY_W]) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S]) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A]) camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_D]) camera.ProcessKeyboard(RIGHT, deltaTime);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) keys[key] = true;
        else if (action == GLFW_RELEASE) keys[key] = false;
    }
}

void MouseCallback(GLFWwindow* window, double xPos, double yPos) {
    if (firstMouse) { lastX = (GLfloat)xPos; lastY = (GLfloat)yPos; firstMouse = false; }
    GLfloat xOffset = (GLfloat)xPos - lastX;
    GLfloat yOffset = lastY - (GLfloat)yPos;
    lastX = (GLfloat)xPos; lastY = (GLfloat)yPos;
    camera.ProcessMouseMovement(xOffset, yOffset);
}