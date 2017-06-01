#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"

#define GLM_FORCE_RADIANS
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/epsilon.hpp>

#define _VR

using namespace std;
using namespace glm;

#ifdef _VR
#include "minimalOpenVR.h"
vr::IVRSystem* hmd = nullptr;
#endif

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from
shared_ptr<Program> progP;
shared_ptr<Program> progE;
shared_ptr<Program> progF;
shared_ptr<Shape> bunnyMesh;
shared_ptr<Shape> ground;
shared_ptr<Shape> sphere;

int g_width, g_height;
float sTheta;
float delta = 0.01f;

vec3 lightPos(-2.0f, 2.0f, 2.0f);
vec3 lightColor(1.0f, 1.0f, 1.0f);
vec3 cameraRot(0.0f, 0.0f, 0.0f);
vec3 eye(0.0f, 1.0f, 0.0f);
vec3 up(0.0f, 1.0f, 0.0f);

vec3 lookAtVec = vec3(cos(cameraRot.y) * cos(cameraRot.x), sin(cameraRot.y), cos(cameraRot.y) * cos(3.14f / 2.0f - cameraRot.x)) + eye;
vec3 gaze = lookAtVec - eye;
vec3 camW = (-1.0f * gaze) / length(gaze);
vec3 camU = cross(up, camW) / length(cross(up, camW));
vec3 camV = cross(camW, camU);

static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

//helper function to set materials
void SetMaterial(int i) {
	switch (i) {
	case 0: //shiny midnight blue
		glUniform3f(progP->getUniform("MatAmb"), 0.01, 0.01, 0.01);
		glUniform3f(progP->getUniform("MatDif"), 0.0, 0.01, 0.2);
		glUniform3f(progP->getUniform("MatSpec"), 0.01, 0.01, 1.0);
		glUniform1f(progP->getUniform("shine"), 120.0);
		break;
	case 1: // flat grey
		glUniform3f(progP->getUniform("MatAmb"), 0.13, 0.13, 0.14);
		glUniform3f(progP->getUniform("MatDif"), 0.3, 0.3, 0.4);
		glUniform3f(progP->getUniform("MatSpec"), 0.3, 0.3, 0.4);
		glUniform1f(progP->getUniform("shine"), 4.0);
		break;
	case 2: // matte red
		glUniform3f(progP->getUniform("MatAmb"), 0.3294, 0.0, 0.0);
		glUniform3f(progP->getUniform("MatDif"), 0.74, 0.2863, 0.0);
		glUniform3f(progP->getUniform("MatSpec"), 0.6, 0.0, 0.0);
		glUniform1f(progP->getUniform("shine"), 4.0);
		break;
	case 3: // shiny cloud
		glUniform3f(progP->getUniform("MatAmb"), 0.72, 0.72, 0.72);
		glUniform3f(progP->getUniform("MatDif"), 0.2, 0.2, 0.72);
		glUniform3f(progP->getUniform("MatSpec"), 0.72, 0.72, 0.72);
		glUniform1f(progP->getUniform("shine"), 128);
		break;
	case 4: // shiny seafoam green
		glUniform3f(progP->getUniform("MatAmb"), 0.2, 0.1, 0.2);
		glUniform3f(progP->getUniform("MatDif"), 0.0, 0.46, 0.2);
		glUniform3f(progP->getUniform("MatSpec"), 0.9, 0.2, 0.2);
		glUniform1f(progP->getUniform("shine"), 110.0);
		break;
	}
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	GLfloat cameraSpeed = 0.05f;

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key == GLFW_KEY_Q) {
		lightPos -= vec3(0.1f, 0.0f, 0.0f);
	}

	if (key == GLFW_KEY_E) {
		lightPos += vec3(0.1f, 0.0f, 0.0f);
	}

	if (key == GLFW_KEY_W) {
		lookAtVec -= cameraSpeed * camW;
		eye -= cameraSpeed * camW;
	}
	if (key == GLFW_KEY_A) {
		lookAtVec -= cameraSpeed * camU;
		eye -= cameraSpeed * camU;
	}
	if (key == GLFW_KEY_S) {
		lookAtVec += cameraSpeed * camW;
		eye += cameraSpeed * camW;
	}
	if (key == GLFW_KEY_D) {
		lookAtVec += cameraSpeed * camU;
		eye += cameraSpeed * camU;
	}
	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		vr::VRSystem()->ResetSeatedZeroPose();
	}
}

static void scroll_callback(GLFWwindow *window, double offsetX, double offsetY) {
	cameraRot.x += ((float)offsetX / g_width) * 3.14f;
	cameraRot.y += ((float)offsetY / g_height) * 3.14f;
	if (cameraRot.y >= 1.396f) {
		cameraRot.y = 1.396f;
	}
	else if (cameraRot.y <= -1.396f) {
		cameraRot.y = -1.396f;
	}
}

static void mouse_callback(GLFWwindow *window, int button, int action, int mods) {
	double posX, posY;

	if (action == GLFW_PRESS) {
		glfwGetCursorPos(window, &posX, &posY);
		cout << "Pos X " << posX << " Pos Y " << posY << endl;
	}
}

static void resize_callback(GLFWwindow *window, int width, int height) {
	g_width = width;
	g_height = height;
	glViewport(0, 0, width, height);
}

static void init()
{
	GLSL::checkVersion();

	sTheta = 0;

	// Set background color.
	glClearColor(.12f, .34f, .56f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	// Initialize bunny mesh.
	bunnyMesh = make_shared<Shape>();
	bunnyMesh->loadMesh(RESOURCE_DIR + "bunny.obj");
	bunnyMesh->resize();
	bunnyMesh->init();

	// Initialize bunny mesh.
	ground = make_shared<Shape>();
	ground->loadMesh(RESOURCE_DIR + "cube.obj");
	ground->resize();
	ground->init();

	// Initialize sphere mesh.
	sphere = make_shared<Shape>();
	sphere->loadMesh(RESOURCE_DIR + "smooth_sphere.obj");
	sphere->resize();
	sphere->init();

	progP = make_shared<Program>();
	progP->setVerbose(true);
	progP->setShaderNames(RESOURCE_DIR + "phong_vert.glsl", RESOURCE_DIR + "phong_frag.glsl");
	progP->init();
	progP->addUniform("P");
	progP->addUniform("M");
	progP->addUniform("V");
	progP->addUniform("lightColor");
	progP->addUniform("lightPos");
	progP->addAttribute("vertPos");
	progP->addAttribute("vertNor");
	progP->addUniform("MatAmb");
	progP->addUniform("MatDif");
	progP->addUniform("MatSpec");
	progP->addUniform("shine");

	// eyes
	progE = make_shared<Program>();
	progE->setVerbose(true);
	progE->setShaderNames(RESOURCE_DIR + "eye_vert.glsl", RESOURCE_DIR + "eye_frag.glsl");
	progE->init();
	progE->addUniform("P");
	progE->addUniform("M");
	progE->addUniform("V");
	progE->addAttribute("vertPos");
	progE->addAttribute("vertNor");

	// face features
	progF = make_shared<Program>();
	progF->setVerbose(true);
	progF->setShaderNames(RESOURCE_DIR + "face_vert.glsl", RESOURCE_DIR + "face_frag.glsl");
	progF->init();
	progF->addUniform("P");
	progF->addUniform("M");
	progF->addUniform("V");
	progF->addAttribute("vertPos");
	progF->addAttribute("vertNor");
}

static void drawBunny(shared_ptr<MatrixStack> P, mat4 V, shared_ptr<MatrixStack>* M, float transX, float transZ, int c) {
	// body and head
	(*M)->loadIdentity();
	(*M)->translate(vec3(transX, 1.0f, transZ));
	(*M)->translate(vec3(0.0f, -0.8f, 0.0f));
	(*M)->scale(vec3(0.8f, 1.2f, 0.8f));

	progP->bind();
	SetMaterial(c);
	glUniformMatrix4fv(progP->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progP->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr((*M)->topMatrix()));
	sphere->draw(progP);
	progP->unbind();

	// left ear
	(*M)->loadIdentity();
	(*M)->translate(vec3(transX, 1.0f, transZ));
	(*M)->translate(vec3(-0.7f, 1.0f, 0.0f));
	(*M)->rotate(0.2, vec3(0, 0, 1));
	(*M)->scale(vec3(0.5f, 0.9f, 0.5f));

	progP->bind();
	SetMaterial(c);
	glUniformMatrix4fv(progP->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progP->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr((*M)->topMatrix()));
	sphere->draw(progP);
	progP->unbind();

	// right ear
	(*M)->loadIdentity();
	(*M)->translate(vec3(transX, 1.0f, transZ));
	(*M)->translate(vec3(0.7f, 1.0f, 0.0f));
	(*M)->rotate(sTheta, vec3(0, 0, 1));
	(*M)->scale(vec3(0.5f, 0.9f, 0.5f));

	progP->bind();
	SetMaterial(c);
	glUniformMatrix4fv(progP->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progP->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr((*M)->topMatrix()));
	sphere->draw(progP);
	progP->unbind();

	// left foot
	(*M)->loadIdentity();
	(*M)->translate(vec3(transX, 1.0f, transZ));
	(*M)->translate(vec3(-.6, -1.8, 0));
	(*M)->rotate(0.2, vec3(0, 0, 1));
	(*M)->scale(vec3(.3, .3, .3));

	progP->bind();
	SetMaterial(c);
	glUniformMatrix4fv(progP->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progP->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr((*M)->topMatrix()));
	sphere->draw(progP);
	progP->unbind();

	// right foot
	(*M)->loadIdentity();
	(*M)->translate(vec3(transX, 1.0f, transZ));
	(*M)->translate(vec3(0.6f, -1.8f, 0.0f));
	(*M)->rotate(0.2, vec3(0, 0, 1));
	(*M)->scale(vec3(0.3f, 0.3f, 0.3f));

	progP->bind();
	SetMaterial(c);
	glUniformMatrix4fv(progP->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progP->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr((*M)->topMatrix()));
	sphere->draw(progP);
	progP->unbind();

	// left hand
	(*M)->loadIdentity();
	(*M)->translate(vec3(transX, 1.0f, transZ));
	(*M)->translate(vec3(-0.6f, -0.8f, 0.0f));
	(*M)->rotate(0.2, vec3(0, 0, 1));
	(*M)->scale(vec3(0.4f, 0.3f, 0.4f));

	progP->bind();
	SetMaterial(c);
	glUniformMatrix4fv(progP->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progP->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr((*M)->topMatrix()));
	sphere->draw(progP);
	progP->unbind();

	// right hand
	(*M)->loadIdentity();
	(*M)->translate(vec3(transX, 1.0f, transZ));
	(*M)->translate(vec3(0.6f, -0.8f, 0.0f));
	(*M)->rotate(0.2, vec3(0, 0, 1));
	(*M)->scale(vec3(0.4f, 0.3f, 0.4f));

	progP->bind();
	SetMaterial(c);
	glUniformMatrix4fv(progP->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progP->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr((*M)->topMatrix()));
	sphere->draw(progP);
	progP->unbind();

	// left eye
	(*M)->loadIdentity();
	(*M)->translate(vec3(transX, 1.0f, transZ));
	(*M)->translate(vec3(-0.3f, -0.3f, 0.6f));
	(*M)->scale(vec3(0.2f, 0.3f, 0.2f));

	progE->bind();
	glUniformMatrix4fv(progE->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progE->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progE->getUniform("M"), 1, GL_FALSE, value_ptr((*M)->topMatrix()));
	sphere->draw(progE);
	progE->unbind();

	(*M)->loadIdentity();
	(*M)->translate(vec3(transX, 1.0f, transZ));
	(*M)->translate(vec3(-0.3f, -0.3f, 0.8f));
	(*M)->scale(vec3(0.1f, 0.2f, 0.1f));

	progF->bind();
	glUniformMatrix4fv(progF->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progF->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progF->getUniform("M"), 1, GL_FALSE, value_ptr((*M)->topMatrix()));
	sphere->draw(progF);
	progF->unbind();

	// right eye
	(*M)->loadIdentity();
	(*M)->translate(vec3(transX, 1.0f, transZ));
	(*M)->translate(vec3(0.3f, -0.3f, 0.6f));
	(*M)->scale(vec3(0.2f, 0.3f, 0.2f));

	progE->bind();
	glUniformMatrix4fv(progE->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progE->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progE->getUniform("M"), 1, GL_FALSE, value_ptr((*M)->topMatrix()));
	sphere->draw(progE);
	progE->unbind();

	(*M)->loadIdentity();
	(*M)->translate(vec3(transX, 1.0f, transZ));
	(*M)->translate(vec3(0.3f, -0.3f, 0.8f));
	(*M)->scale(vec3(0.1f, 0.2f, 0.1f));

	progF->bind();
	glUniformMatrix4fv(progF->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progF->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progF->getUniform("M"), 1, GL_FALSE, value_ptr((*M)->topMatrix()));
	sphere->draw(progF);
	progF->unbind();

	// nose
	(*M)->loadIdentity();
	(*M)->translate(vec3(transX, 1.0f, transZ));
	(*M)->translate(vec3(0.0f, -0.7f, 0.9f));
	(*M)->scale(vec3(0.1f, 0.1f, 0.1f));

	progF->bind();
	glUniformMatrix4fv(progF->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progF->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progF->getUniform("M"), 1, GL_FALSE, value_ptr((*M)->topMatrix()));
	sphere->draw(progF);
	progF->unbind();

	// tail
	(*M)->loadIdentity();
	(*M)->translate(vec3(transX, 1.0f, transZ));
	(*M)->translate(vec3(0.0f, -1.3f, -0.8f));
	(*M)->scale(vec3(0.2f, 0.2f, 0.2f));

	progE->bind();
	glUniformMatrix4fv(progE->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progE->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progE->getUniform("M"), 1, GL_FALSE, value_ptr((*M)->topMatrix()));
	sphere->draw(progE);
	progE->unbind();
}

mat4 eyeToHead[2], projectionMatrix[2], headToBodyMatrix, tHeadToBodyMatrix, tProjectionMatrix[2], tEyeToHead[2];

static void render(int neye)
{
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	g_height = height;
	g_width = width;
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Use the matrix stack for Lab 6
	float aspect = width / (float)height;

	// Create the matrix stacks - please leave these alone for now
	auto P = make_shared<MatrixStack>();
	auto M = make_shared<MatrixStack>();
	auto V = glm::mat4(1);
	// Apply perspective projection.
	P->pushMatrix();
	P->loadIdentity();
	P->multMatrix(tProjectionMatrix[neye]);
	//P->perspective(45.0f, aspect, 0.01f, 100.0f);
	//lookAtVec = vec3(cos(cameraRot.y) * cos(cameraRot.x), sin(cameraRot.y), cos(cameraRot.y) * cos(3.14f / 2.0f - cameraRot.x)) + eye;
	//gaze = lookAtVec - eye;

	//camW = (-1.0f * gaze) / length(gaze);
	//camU = cross(up, camW) / length(cross(up, camW));
	//camV = cross(camW, camU);


	//V = headToBodyMatrix * eyeToHead[neye];
	V = tEyeToHead[neye] * tHeadToBodyMatrix;
	float *temp = value_ptr(V);
	cout << temp[3] << ", " << temp[7] << ", " << temp[11] << ", " << temp[15] << endl;
	assert(temp[3] == 0 && temp[7] == 0 && temp[11] == 0 && temp[15] == 1);
	//V = glm::lookAt(eye, lookAtVec, up);

	// Draw a stack of cubes with indiviudal transforms 
	progP->bind();
	glUniformMatrix4fv(progP->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(progP->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniform3f(progP->getUniform("lightColor"), lightColor.x, lightColor.y, lightColor.z);
	glUniform3f(progP->getUniform("lightPos"), lightPos.x, lightPos.y, lightPos.z);

	// draw mesh bunnies
	{
		M->pushMatrix();
		SetMaterial(3);
		M->loadIdentity();
		M->translate(vec3(5, 0, 0));
		M->scale(vec3(0.75, 0.75, 0.75));
		glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		bunnyMesh->draw(progP);

		SetMaterial(0);
		M->loadIdentity();
		M->translate(vec3(-5, 0, -5));
		M->scale(vec3(0.75, 0.75, 0.75));
		glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		bunnyMesh->draw(progP);

		SetMaterial(2);
		M->loadIdentity();
		M->translate(vec3(-25, 0, 5));
		M->scale(vec3(0.75, 0.75, 0.75));
		glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		bunnyMesh->draw(progP);

		SetMaterial(1);
		M->loadIdentity();
		M->translate(vec3(25, 0, 5));
		M->scale(vec3(0.75, 0.75, 0.75));
		glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		bunnyMesh->draw(progP);

		SetMaterial(2);
		M->loadIdentity();
		M->translate(vec3(7, 0, 2));
		M->scale(vec3(0.75, 0.75, 0.75));
		glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		bunnyMesh->draw(progP);

		SetMaterial(0);
		M->loadIdentity();
		M->translate(vec3(-7, 0, -7));
		M->scale(vec3(0.75, 0.75, 0.75));
		glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		bunnyMesh->draw(progP);

		SetMaterial(0);
		M->loadIdentity();
		M->translate(vec3(-7, 0, 17));
		M->scale(vec3(0.75, 0.75, 0.75));
		glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		bunnyMesh->draw(progP);

		SetMaterial(3);
		M->loadIdentity();
		M->translate(vec3(7, 0, 17));
		M->scale(vec3(0.75, 0.75, 0.75));
		glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		bunnyMesh->draw(progP);

		SetMaterial(1);
		M->loadIdentity();
		M->translate(vec3(15, 0, 15));
		M->scale(vec3(0.75, 0.75, 0.75));
		glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		bunnyMesh->draw(progP);

		SetMaterial(0);
		M->loadIdentity();
		M->translate(vec3(-15, 0, -15));
		M->scale(vec3(0.75, 0.75, 0.75));
		glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		bunnyMesh->draw(progP);

		// draw ground
		SetMaterial(4);
		M->translate(vec3(0, -5, 0));
		M->scale(vec3(200, .1, 200));
		glUniformMatrix4fv(progP->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		ground->draw(progP);
	}
	progP->unbind();

	M->popMatrix();

	// Pop matrix stacks.
	P->popMatrix();

	sTheta -= delta;
	if (sTheta < -0.6f) {
		sTheta = -0.6f;
		delta = -delta;
	}
	else if (sTheta > -0.1f) {
		sTheta = -0.1f;
		delta = -delta;
	}
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		cout << "Please specify the resource directory." << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");

	uint32_t framebufferWidth = 1280, framebufferHeight = 720;
#   ifdef _VR
	const int numEyes = 2;
	hmd = initOpenVR(framebufferWidth, framebufferHeight);
	assert(hmd);
#   else
	const int numEyes = 1;
#   endif

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if (!glfwInit()) {
		return -1;
	}
	//request the highest possible version of OGL - important for mac
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(framebufferWidth, framebufferHeight, "Project04VR", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	//weird bootstrap of glGetError
	glGetError();
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	//set the mouse call back
	glfwSetMouseButtonCallback(window, mouse_callback);
	//set the window resize call back
	glfwSetFramebufferSizeCallback(window, resize_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Initialize scene. Note geometry initialized in init now
	init();

	GLuint framebuffer[numEyes];
	glGenFramebuffers(numEyes, framebuffer);

	GLuint colorRenderTarget[numEyes], depthRenderTarget[numEyes];
	glGenTextures(numEyes, colorRenderTarget);
	glGenTextures(numEyes, depthRenderTarget);
	for (int eye = 0; eye < numEyes; ++eye) {
		glBindTexture(GL_TEXTURE_2D, colorRenderTarget[eye]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferWidth, framebufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		glBindTexture(GL_TEXTURE_2D, depthRenderTarget[eye]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, framebufferWidth, framebufferHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer[eye]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorRenderTarget[eye], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthRenderTarget[eye], 0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

#   ifdef _VR
	vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
#   endif

	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(window)) {
#ifdef _VR
		getEyeTransformations(
			hmd,
			trackedDevicePose,
			-0.1f,
			-200.0f,
			value_ptr(headToBodyMatrix),
			value_ptr(eyeToHead[0]),
			value_ptr(eyeToHead[1]),
			value_ptr(projectionMatrix[0]),
			value_ptr(projectionMatrix[1])
		);

		assert(all(epsilonEqual(headToBodyMatrix[3], vec4(0.0f, 0.0f, 0.0f, 1.0f), 0.0001f)));
		assert(all(epsilonEqual(eyeToHead[0][3], vec4(0.0f, 0.0f, 0.0f, 1.0f), 0.0001f)));
		assert(all(epsilonEqual(eyeToHead[1][3], vec4(0.0f, 0.0f, 0.0f, 1.0f), 0.0001f)));

		tHeadToBodyMatrix = transpose(headToBodyMatrix);
		tEyeToHead[0] = transpose(eyeToHead[0]);
		tEyeToHead[1] = transpose(eyeToHead[1]);
		tProjectionMatrix[0] = transpose(projectionMatrix[0]);
		tProjectionMatrix[1] = transpose(projectionMatrix[1]);

#else
		projectionMatrix[0] = perspective(45.0f, (float)framebufferWidth / framebufferHeight, 0.1f, 100.0f);
#       endif

		for (int eye = 0; eye < numEyes; eye++) {
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer[eye]);
			glViewport(0, 0, framebufferWidth, framebufferHeight);


			// Render scene.
			render(eye);

#           ifdef _VR
			{
				const vr::Texture_t tex = { reinterpret_cast<void*>(intptr_t(colorRenderTarget[eye])), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
				vr::VRCompositor()->Submit(vr::EVREye(eye), &tex);
			}
#           endif
		}
#       ifdef _VR
		// Tell the compositor to begin work immediately instead of waiting for the next WaitGetPoses() call
		vr::VRCompositor()->PostPresentHandoff();
#       endif

		// Mirror to the window
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_NONE);
		glViewport(0, 0, framebufferWidth, framebufferHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBlitFramebuffer(0, 0, framebufferWidth, framebufferHeight, 0, 0, framebufferWidth, framebufferHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, GL_NONE);

		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}

#   ifdef _VR
	if (hmd != nullptr) {
		vr::VR_Shutdown();
	}
#   endif

	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
