#define GLEW_STATIC

#include <iostream>
#include "glm/glm.hpp"//core glm functionality
#include "glm/gtc/matrix_transform.hpp"//glm extension for generating common transformation matrices
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "GLEW/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include<stdio.h>
#include "Shader.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"
#include<time.h>

#define TINYOBJLOADER_IMPLEMENTATION
#define SHADOW_WIDTH 2048
#define SHADOW_HEIGHT 2048

#include "Model3D.hpp"
#include "Mesh.hpp"


int glWindowWidth = 640, glWindowHeight = 480, retina_width, retina_height;
double lastX = glWindowWidth / 2, lastY = glWindowHeight / 2;
bool firstMouse = true, pressedKeys[1024] ;
float yaw = -90.0f, pitch = 0.0f, lightAngle, cameraSpeed = 0.01f, angle = 0.0f;
GLFWwindow* glWindow = NULL;
glm::vec3 lightDir, lightColor;
glm::mat3 normalMatrix, lightDirMatrix;
glm::mat4 model, view, projection, viewLight, lightProjection, lightSpaceTrMatrix;
GLuint modelLoc, viewLoc, projectionLoc, normalMatrixLoc, shadowMapFBO, depthMapTexture, lightDirLoc, lightColorLoc, lightDirMatrixLoc, textureID;
const GLfloat near_plane = 1.0f, far_plane = 30.0f;
gps::Camera myCamera(glm::vec3(0.0f, 1.0f, 2.5f), glm::vec3(0.0f, 0.0f, 0.0f));
gps::SkyBox mySkyBox;
gps::Shader myCustomShader, depthMapShader, skyBoxShader, carShader;
std::vector<const GLchar*> faces;
glm::vec3 positionBidirectional(0.0f, 1.0f, 2.0f);
//Model3D
gps::Model3D plane;
gps::Model3D road_plane;
gps::Model3D city;
gps::Model3D ground;
bool start_plane = false;
float up_plane=0.0;
float distance = 0.0;
float up_distance=0.0f;
float fog_coefficient = 0.0f;
float fogFcator = 0.0f;
clock_t time1 = clock();

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}

void windowResizeCallback(GLFWwindow* window, int width, int height)
{
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//TODO
	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	//set projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	//send matrix data to shader
	GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set Viewport transform
	glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}



void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	//int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	//if (state != GLFW_PRESS)
//	{
	//	firstMouse = true;
	///	return;
	//}
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}
	float xOffset = (lastX - xpos)*0.1;
	float yOffset = (ypos - lastY)*0.1;
	lastX = xpos;
	lastY = ypos;
	yaw -= xOffset;
	pitch -= yOffset;
	myCamera.rotate(pitch, yaw);
}
glm::mat4 computeLightSpaceTrMatrix()
{
	const GLfloat near_plane = 1.0f, far_plane = 10.0f;
	glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);

	glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(positionBidirectional, 1.0f));
	glm::mat4 lightView = glm::lookAt(lightDirTr, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

	return lightProjection * lightView;
}

void processMovement()
{
	//Polygonal and wireframe
	if (pressedKeys[GLFW_KEY_B])
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	if (pressedKeys[GLFW_KEY_N])
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	if (pressedKeys[GLFW_KEY_M])
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}
	//smooth
	
	//Game logic
	if(pressedKeys[GLFW_KEY_T])
	{
		start_plane = true;
	}
	if (pressedKeys[GLFW_KEY_Y])
	{
		start_plane = false;
	}
	if (pressedKeys[GLFW_KEY_U])
	{
		up_plane = 1.0f;
	}
	if (pressedKeys[GLFW_KEY_I])
	{
		up_plane = 2.0f;
	}
	if (pressedKeys[GLFW_KEY_O])
	{
		up_plane=0.0f;
	}
	if (pressedKeys[GLFW_KEY_Q]) {
		angle += 0.1f;
		if (angle > 360.0f)
			angle -= 360.0f;
	}

	if (pressedKeys[GLFW_KEY_E]) {
		angle -= 0.1f;
		if (angle < 0.0f)
			angle += 360.0f;
	}

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
	}
	if (pressedKeys[GLFW_KEY_J]) {
		lightAngle -= 0.5f;
		if (lightAngle < 0.0f)
			lightAngle = 360.0f;
		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f))*glm::vec4(positionBidirectional, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}
	if (pressedKeys[GLFW_KEY_K]) {
		lightAngle += 0.5f;
		if (lightAngle > 360.f)
			lightAngle = 0.0f;
		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f))*glm::vec4(positionBidirectional, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}
}
void plane_movement() {
	if (start_plane)
		distance+=0.1;
	if (up_plane == 1.0)
	{
		up_distance += 0.1;
	}
	else if (up_plane == 2.0f)
	{
		up_distance -= 0.1;
	}
	else {

	}
}
bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	//for Mac OS X
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwMakeContextCurrent(glWindow);

	glfwWindowHint(GLFW_SAMPLES, 4);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
	
	//glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	return true;
}

void initOpenGLState()
{
glClearColor(0.3, 0.3, 0.3, 1.0);
glViewport(0, 0, retina_width, retina_height);

glEnable(GL_DEPTH_TEST); // enable depth-testing
glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
glEnable(GL_CULL_FACE); // cull face
glCullFace(GL_BACK); // cull back face
glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels()
{
	plane = gps::Model3D("objects/plane/Futuristic combat jet.obj", "objects/plane/");
	road_plane = gps::Model3D("objects/Road/roadV1.obj", "objects/Road/");
	city = gps::Model3D("objects/City/Street environment_V01.obj", "objects/City/");
	ground = gps::Model3D("objects/ground/ground.obj", "objects/ground/");
}

void initShaders()
{
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	depthMapShader.loadShader("shaders/shaderShadow.vert", "shaders/shaderShadow.frag");
	depthMapShader.useShaderProgram();
	skyBoxShader.loadShader("shaders/skyBoxShader.vert", "shaders/skyBoxShader.frag");
	skyBoxShader.useShaderProgram();

}

void initSkyBox() {
	faces.push_back("skybox/urbansp_rt.tga");
	faces.push_back("skybox/urbansp_lf.tga");
	faces.push_back("skybox/urbansp_up.tga");
	faces.push_back("skybox/urbansp_dn.tga");
	faces.push_back("skybox/urbansp_bk.tga");
	faces.push_back("skybox/urbansp_ft.tga");
	mySkyBox.Load(faces);
}
//locatiile de memoie pt shader
void initUniforms()
{
	myCustomShader.useShaderProgram();
	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 3000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 2.0f);
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "dirLight.direction");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
}
//frame buffer object
void initFBO() {
	glGenFramebuffers(1, &shadowMapFBO);
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

glm::mat4 computeLightSpaceMatrix() {
	const GLfloat near_plane = 1.0f, far_plane = 10.0f;
	glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
	glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
	glm::mat4 lightView = glm::lookAt(lightDirTr, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

	return lightProjection * lightView;
}


void processFog() {
	clock_t t1 = clock();
	myCustomShader.useShaderProgram();
	float diff= ((float)t1 - (float)time1)/CLOCKS_PER_SEC;
	if (diff < 20)
	{
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "distanceFog"), fogFcator);
	}
	else
		if (diff > 20 && diff < 30)
		{
				fogFcator += 0.00002;
			glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "distanceFog"), fogFcator);
		}
		else
	if (diff > 30 && diff < 40)
	{
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "distanceFog"), fogFcator);
	}
	else
		if (diff > 40 && diff < 50)
		{
			fogFcator -= 0.00002;
			glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "distanceFog"), fogFcator);
			
		}else

			if (diff > 50)
			{
				time1 = t1;
				fogFcator = 0.0f;
			}
	
	
}
void renderScene()
{
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	plane_movement();
	processMovement();
	processFog();
	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);


	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5, 1.0, 2.0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


	//Drawing
	//hartile de adancime

	//First Road Plane
	road_plane.Draw(depthMapShader);

	//Second Road Plane
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, 3000));
	model = glm::scale(model, glm::vec3(0.5, 1.0, 2.0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	road_plane.Draw(depthMapShader);
	//Plane
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-10, up_distance, distance));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	plane.Draw(depthMapShader);

	//City Drawing 1
	model = glm::translate(glm::mat4(1.0f), glm::vec3(52, 0.0, 0.0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(depthMapShader);
	//City Drawing 1.a
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(depthMapShader);
	//City Drawing 1.b
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(depthMapShader);
	//City Drawing 1.c
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(depthMapShader);
	//City Drawing 1.d
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(depthMapShader);
	//City Drawing 2
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-72, 0.0, 0.0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(depthMapShader);
	//City Drawing 2.a
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(depthMapShader);
	//City Drawing 2.b
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(depthMapShader);
	//City Drawing 2.c
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(depthMapShader);
	//City Drawing 2.d
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(depthMapShader);

	//Ground

	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.1f, 250.0f));
	model = glm::scale(model, glm::vec3(12.0f, 1.0f, 17.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	ground.Draw(depthMapShader);


	//create model matrix for nanosuit
	

	
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//send model matrix data to shader	
	
	glViewport(0, 0, retina_width, retina_height);

	myCustomShader.useShaderProgram();





	//Iluminating and matrixes
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));


	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	lightDirMatrix = glm::mat3(glm::inverseTranspose(view));
	glUniformMatrix3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightDirMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceMatrix()));


	glm::vec3 pointLightPositions[] = {
	glm::vec3(9.5f,  1.0f,  -2.0f),
	glm::vec3(-11.5f, 1.0f, -2.0f),
	glm::vec3(9.5f,  1.0f, 10.0f),
	glm::vec3(-11.5f,  1.0f, 10.0f),
	glm::vec3(9.5f,  1.0f,  40.0f),
	glm::vec3(-11.5f, 1.0f,40.0f),
	glm::vec3(9.5f,  1.0f, 90.0f),
	glm::vec3(-11.5f,  1.0f, 90.0f),
	glm::vec3(-11.5f,  1.0f, 100.0f)
	};

	glClearColor(0.75f, 0.52f, 0.3f, 1.0f);
	//Just white color
	//luminile punctiforme si directional light
	glm::vec3 pointLightColors[] = {
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, 1.0, 1.0),
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, 1.0, 1.0),
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 1.0f)
	};

	glUniform3f(glGetUniformLocation(myCustomShader.shaderProgram, "dirLight.position"), positionBidirectional.x, positionBidirectional.y, positionBidirectional.z);
	glUniform3f(glGetUniformLocation(myCustomShader.shaderProgram, "dirLight.ambient"), 0.5f, 0.5f, 0.5f);
	glUniform3f(glGetUniformLocation(myCustomShader.shaderProgram, "dirLight.diffuse"), 0.5f, 0.5f, 0.5f);
	glUniform3f(glGetUniformLocation(myCustomShader.shaderProgram, "dirLight.specular"), 0.9f, 0.9f, 0.9f);
	for (GLuint i = 0; i < 9; i++)
	{
		std::string number = std::to_string(i);

		glUniform3f(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].position").c_str()), pointLightPositions[i].x, pointLightPositions[i].y, pointLightPositions[i].z);
		glUniform3f(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].ambient").c_str()), pointLightColors[i].x * 0.1f, pointLightColors[i].y * 0.1f, pointLightColors[i].z * 0.1f);
		glUniform3f(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].diffuse").c_str()), pointLightColors[i].x, pointLightColors[i].y, pointLightColors[i].z);
		glUniform3f(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].specular").c_str()), 1.0f, 1.0f, 1.0f);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].constant").c_str()), 1.0f);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].linear").c_str()), 0.09f);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].quadratic").c_str()), 0.032f);

	}





	view = myCamera.getViewMatrix();

	//send view matrix data to shader	
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceMatrix()));
	model = glm::mat4(1.0f);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//initialize the view matrix
	view = myCamera.getViewMatrix();

	//send view matrix data to shader	
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	model = glm::scale(model, glm::vec3(0.5, 1.0, 2.0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


	//Drawing


	//First Road Plane
	road_plane.Draw(myCustomShader);

	//Second Road Plane
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, 3000));
	model = glm::scale(model, glm::vec3(0.5, 1.0, 2.0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	road_plane.Draw(myCustomShader);
	//Plane
	model = glm::translate(glm::mat4(1.0f) , glm::vec3(-10, up_distance, distance));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	plane.Draw(myCustomShader);

	//City Drawing 1
	model = glm::translate(glm::mat4(1.0f), glm::vec3(52, 0.0, 0.0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(myCustomShader);
	//City Drawing 1.a
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(myCustomShader);
	//City Drawing 1.b
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(myCustomShader);
	//City Drawing 1.c
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(myCustomShader);
	//City Drawing 1.d
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(myCustomShader);
	//City Drawing 2
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-72, 0.0, 0.0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(myCustomShader);
	//City Drawing 2.a
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(myCustomShader);
	//City Drawing 2.b
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(myCustomShader);
	//City Drawing 2.c
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(myCustomShader);
	//City Drawing 2.d
	model = glm::translate(model, glm::vec3(0.0, 0.0, 94));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	city.Draw(myCustomShader);

	//Ground
	
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.1f, 250.0f));
	model = glm::scale(model, glm::vec3(12.0f, 1.0f, 17.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	ground.Draw(myCustomShader);


	//create normal matrix
	
	

	

	skyBoxShader.useShaderProgram();

	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyBoxShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(skyBoxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	mySkyBox.Draw(skyBoxShader, view, projection);
}

int main(int argc, const char * argv[]) {

	initOpenGLWindow();
	initOpenGLState();
	initFBO();
	initModels();
	initSkyBox();
	initShaders();
	initUniforms();

	while (!glfwWindowShouldClose(glWindow)) {


		renderScene();

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	//close GL context and any other GLFW resources
	glfwTerminate();

	return 0;
}
