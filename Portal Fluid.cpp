#include <iostream>
#include "glad.h"
#include <glfw3.h>
#include "Shader.h"
#include <vector>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/random.hpp"
#include "glm/gtx/quaternion.hpp"
#include "Shader.h"


#define MAX_PARTICLES 10
#define DEADLINE 0

struct Particle
{
	glm::vec3 velocity;
	glm::vec3 position;
};

enum DisplayMode
{
	displayParticles,
	displayBillboards,
	linearMode,
	defaultMode
};

//screen size variables
int screenWidth = 800, screenHeight = 600;

//helper variables to control rotation by mouse
float lastX, lastY;
bool unSet = true;
bool mouseButtonPressed = false;

glm::quat orientationQuat = glm::quat(1, 0, 0, 0);

//helper variables to control the switching of display modes
DisplayMode mode = defaultMode;
bool linearFunction = false;

//gravity variable for Euler integration and simluation time alteration
float Gravity = 2.5f;

//camera vectors
glm::vec3 cameraPos = glm::vec3(4, 3, 1);
glm::vec3 cameraUp = glm::vec3(0, 1, 0);

//Matrices
glm::mat4 Model = glm::mat4(1.0f);

glm::mat4 Projection = glm::perspective(glm::radians(90.0f),
                                        static_cast<float>(screenWidth) / static_cast<float>(screenHeight), 0.1f,
                                        100.0f);

//Euler integration helper variable
float m_prevStep = 0;

//callbacks
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);

//particle storage
std::vector<Particle> particles;
float particleBuffer[3 * MAX_PARTICLES];

//method to create Sphere Coordinates to draw the Sphere! Check definition for more details
void createParticleCoordinates();
Particle generateNewParticle();


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "Portal Fluid", nullptr, nullptr);

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, screenWidth, screenHeight);

	//Tell OpenGL we want to call our previously defined resize function on resize
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_callback);
	glfwSetCursorPosCallback(window, mouse_move_callback);

	//Shader class initialization for the different modes and to have easier access to the shaders in custom files
	Shader particleOnlyShader("EmitterVert.vs", "EmitterFrag.frag");
	
	Shader billboardOnlyShader("ParticleVert.vs", "BillboardOnlyFrag.frag");
	billboardOnlyShader.setGeometryShader("ParticleGeometry.gs");

	Shader ourShader("ParticleVert.vs", "ParticleFrag.frag");
	ourShader.setGeometryShader("ParticleGeometry.gs");
	
	Shader emitterShader("EmitterVert.vs", "EmitterFrag.frag");

	//render the Emitter with lines and required buffer initialization
	//--------------------------------------------------------------------------------
	float vertices[] = {
		0.0f, 2.0f, 1.0f,
		0.0f, 2.0f, -1.0f,
		0.0f, 4.0f, -1.0f,
		0.0f, 4.0f, 1.0f,
		0.0f, 3.0f, 0.0f,
		2.0f, 3.0f, 0.0f,
	};
	unsigned int indices[] = {
		0, 1,
		1, 2,
		2, 3,
		3, 0,
		4, 5,
	};

	//stuff to make the spawner independant from particles and their simulation if needed
	unsigned int visualSpawnVAO, visualSpawnBuffer, EBO;
	glGenBuffers(1, &visualSpawnBuffer);
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &visualSpawnVAO);

	glBindVertexArray(visualSpawnVAO);

	glBindBuffer(GL_ARRAY_BUFFER, visualSpawnBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(nullptr));
	glEnableVertexAttribArray(0);

	//particle system related initializations
	//--------------------------------------------------------------------------------
	//initialize particle buffers
	for (int i = 0; i < MAX_PARTICLES; ++i)
	{
		particles.push_back(generateNewParticle());
	}

	unsigned int buffer, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &buffer);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(particleBuffer), particleBuffer, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(nullptr));
	glEnableVertexAttribArray(0);

	glm::vec3 neighboringParticles[MAX_PARTICLES];

	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		//clear screen
		glClearColor(0.2f, 0.3f, 0.4f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Camera matrix, constructed every frame to accomodate for the mouse movement
		glm::mat4 View = lookAt(
			cameraPos,
			glm::vec3(0, 3, 0),
			cameraUp
		);

		//Emitter rendering
		emitterShader.use();

		glm::mat4 Model = glm::mat4(1.0f);
		glm::mat4 mvp = Projection * View * Model;
		emitterShader.setMat4("MVP", mvp);

		glBindVertexArray(visualSpawnVAO);
		glDrawElements(GL_LINES, 10, GL_UNSIGNED_INT, nullptr);


		//Particle/Fluid Rendering
		if(mode == displayParticles)
		{
			particleOnlyShader.use();
			
			particleOnlyShader.setMat4("MVP", mvp);
		}
		if(mode == displayBillboards)
		{
			billboardOnlyShader.use();
			billboardOnlyShader.setMat4("view", View);
			billboardOnlyShader.setMat4("projection", Projection);
		}
		if(mode == linearMode || mode == defaultMode)
		{
			if (mode == linearMode)
				linearFunction = true;
			ourShader.use();
			ourShader.setMat4("view", View);
			ourShader.setMat4("projection", Projection);

			//neighboring Particles needed for scalarfield computation
			for (int i = 0; i < MAX_PARTICLES; ++i)
			{
				glm::vec4 val = View * glm::vec4(particles.at(i).position, 1);
				neighboringParticles[i] = glm::vec3(val.x, val.y, val.z);
			}
			glUniform3fv(glGetUniformLocation(ourShader.ID, "neighboringParticles"), MAX_PARTICLES, value_ptr(neighboringParticles[0]));

			//lighting related uniforms
			glUniform3fv(glGetUniformLocation(ourShader.ID, "objectColor"), 1, value_ptr(glm::vec3(1, 0.7, 0.2)));
			glUniform3fv(glGetUniformLocation(ourShader.ID, "lightPos"), 1, value_ptr(cameraPos));
			glUniform3fv(glGetUniformLocation(ourShader.ID, "lightColor"), 1, value_ptr(glm::vec3(1, 1, 1)));
			glUniform1f(glGetUniformLocation(ourShader.ID, "shininess"), 16);
			glUniform1f(glGetUniformLocation(ourShader.ID, "specularStrength"), 0.6f);
			glUniform1f(glGetUniformLocation(ourShader.ID, "ambientStrength"), 0.5f);
			glUniform1i(glGetUniformLocation(ourShader.ID, "linearMode"), linearFunction);
			linearFunction = false;
		}
		glBindVertexArray(VAO);

		//fills particleBuffer with updated particle positions
		createParticleCoordinates();
		//update the whole buffer every frame
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(particleBuffer), particleBuffer);
		
		glDrawArrays(GL_POINTS, 0, MAX_PARTICLES);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	//cleanup
	glDeleteVertexArrays(1, &VAO);
	glDeleteVertexArrays(1, &visualSpawnVAO);
	glDeleteBuffers(1, &buffer);
	glDeleteBuffers(1, &visualSpawnBuffer);
	glDeleteBuffers(1, &EBO);

	glfwTerminate();
	return 0;
}

//simulation of particles with simple Euler integration
void createParticleCoordinates()
{
	float nowTime = glfwGetTime();
	float delta = (nowTime - m_prevStep);
	m_prevStep = nowTime;

	if(Gravity != 0)
	{
		for (int i = 0; i < particles.size(); ++i)
		{
			Particle* temp = &particles.at(i);

			temp->velocity += glm::vec3(-Gravity, Gravity, 0) * delta;
			temp->position -= temp->velocity * delta * 0.5f;
			if (temp->position.y < DEADLINE)
			{
				*temp = generateNewParticle();
			}

			particleBuffer[3 * i] = temp->position.x;
			particleBuffer[3 * i + 1] = temp->position.y;
			particleBuffer[3 * i + 2] = temp->position.z;
		}
	}
}

//helper function
Particle generateNewParticle()
{
	return {
		glm::vec3(
			-glm::linearRand(0.001f, 0.01f),
			glm::linearRand(0.001f, 0.01f),
			0),

		glm::vec3(
			glm::linearRand(1.0f, 2.0f),
			glm::linearRand(2.0f, 4.0f),
			glm::linearRand(1.0f, -1.0f)
		)
	};
}

//input callback function
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	
	//Mode switching
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		mode = displayParticles;
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		mode = displayBillboards;
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		mode = linearMode;
	if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		mode = defaultMode;
	
	//simulation time switching
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		Gravity = 2.5f;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		Gravity = 1.0f;
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
		Gravity = 5.0f;
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
		Gravity = 0;
}

//callback function to check if mouse is pressed
void mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		mouseButtonPressed = true;
		//prevent jumping around when mouse button is pressed a second time somewhere else on screen
		unSet = true;
		return;
	}
	mouseButtonPressed = false;
}

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (mouseButtonPressed)
	{
		if (unSet)
		{
			lastX = xpos;
			lastY = ypos;
			unSet = false;
		}
		//Reset rotation when the mouse direction changes to not carry over momentum
		if (xpos < lastX || ypos < lastY || xpos > lastX || ypos > lastY)
			orientationQuat = glm::quat(1, 0, 0, 0);

		float xOffset = xpos - lastX;
		float yOffset = ypos - lastY;
		lastX = xpos;
		lastY = ypos;

		const float sensitivity = 0.2f;
		xOffset *= sensitivity;
		yOffset *= sensitivity;

		glm::vec3 yAxis = glm::vec3(0.0, 3.0, 0.0);
		glm::vec3 xAxis = glm::vec3(1.0, 0.0, 1.0);
		//determining the axis through cross product produces a more natural rotation feeling
		xAxis = normalize(cross(xAxis, yAxis));

		glm::quat yQuat = angleAxis(glm::radians(-yOffset), xAxis);
		glm::quat xQuat = angleAxis(glm::radians(xOffset), yAxis);
		orientationQuat *= xQuat * yQuat;
		normalize(orientationQuat);

		cameraPos = glm::toMat3(orientationQuat) * cameraPos;
	}
}
