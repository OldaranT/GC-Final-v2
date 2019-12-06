#define _USE_MATH_DEFINES
#include <iostream>
#include <vector>
#include <math.h>
#include <cmath>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "glsl.h"
#include "objloader.hpp"
#include "texture.hpp"
#include "ObjectModel.h"


using namespace std;

//--------------------------------------------------------------------------------
// Consts
//--------------------------------------------------------------------------------

const char * fragshader_name = "fragmentshader.fsh";
const char * vertexshader_name = "vertexshader.vsh";
const int WIDTH = 1920, HEIGHT = 1080;
const int DELTA = 10;

const float EYESHEIGHT = 1.75f;
const float MOVEMENTSPEED = 1.0f;

//--------------------------------------------------------------------------------
// Typedefs
//--------------------------------------------------------------------------------
struct LightSource
{
    glm::vec3 position;
};

//--------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------
vector<ObjectModel> objectModels;

GLuint shaderID;

GLuint uniform_mv;
GLuint uniform_apply_texture;
GLuint uniform_material_ambient;
GLuint uniform_material_diffuse;
GLuint uniform_material_specular;
GLuint uniform_material_power;

// Camera
glm::vec3 cameraPos = glm::vec3(0.0, 2.05, 0.0);
glm::vec3 cameraFront = glm::vec3(0.0, 0.0, 0.0);
glm::vec3 cameraUp = glm::vec3(0.0, 1.0, 0.0);
glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));

// Player
const float RESETFACEHEIGHT = 2.05f;
const float RESETWALKINGSPEED = 50.0f;
float walkingSpeed = RESETWALKINGSPEED;
float stepsize = 0.0f; 
float j = 0.0f;
glm::vec3 animationBuffer = glm::vec3();

// Keep track of the delta time
int time;
int oldTime;
float deltaTime;

// Head angle
float yaw = 0.0f, pitch = 0.0f;

glm::mat4 view, projection;

LightSource light;
bool isViewMode = false;

//--------------------------------------------------------------------------------
// Keyboard handling
//--------------------------------------------------------------------------------
void keyboardHandler(unsigned char key, int a, int b)
{
	// Escape
	if (key == 27) {
		glutExit();
	}

	// c = swap between walking mode and view mode
	if (key == 99) {
		if (!isViewMode) {
			cameraPos = glm::vec3(10.0, 14.0, 32.0);
			cameraFront = glm::vec3(0.0, -0.6, -1.0);
			isViewMode = !isViewMode;
		}
		else if (isViewMode) {
			cameraPos = glm::vec3(10.0, RESETFACEHEIGHT, 6.0);
			cameraFront = glm::vec3(0.0, 0.0, -1.0);
			cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));
			isViewMode = !isViewMode;
		}
	}

	if (isViewMode) {
		return;
	}

	walkingSpeed = RESETWALKINGSPEED;
	float cameraSpeed = walkingSpeed * deltaTime;

	// Move forwards, backwards, left and right
	// shift + w (run)
	if (key == 87) {
		walkingSpeed = RESETWALKINGSPEED + 0.1f;
		cameraSpeed = walkingSpeed * deltaTime;

		cameraPos += cameraSpeed * cameraFront;
	}
	// w
	if (key == 119) {
		cameraPos += cameraSpeed * cameraFront;
	}
	// s
	if (key == 115) {
		cameraPos -= cameraSpeed * cameraFront;
	}
	// a
	if (key == 97) {
		cameraPos -= cameraSpeed * cameraRight;
	}
	// d
	if (key == 100) {
		cameraPos += cameraSpeed * cameraRight;
	}

	// look around
	// i
	if (key == 105) {
		float yOffset = 2;
		pitch += yOffset;
	}
	// j
	if (key == 106) {
		float xOffset = -2;
		yaw += xOffset;
		yaw = glm::mod(yaw + xOffset, 360.0f);
	}
	// k
	if (key == 107) {
		float yOffset = -2;
		pitch += yOffset;
	}
	// l
	if (key == 108) {
		float xOffset = 2;
		yaw += xOffset;
		yaw = glm::mod(yaw + xOffset, 360.0f);
	}

	// Only update when one of the "look around"-keys is pressed
	if (key == 105 || key == 106 || key == 107 || key == 108) {
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraFront = glm::normalize(front);
		cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));
	}

	// Keep the camera on ground level
	cameraPos.y = RESETFACEHEIGHT;
}

//--------------------------------------------------------------------------------
// Mouse handling
//--------------------------------------------------------------------------------
void mouseHandler(int x, int y)
{
	if (isViewMode)
		return;

	float xoffset = -((float)(((float)WIDTH / 2.0f) - x));
	float yoffset = ((float)(((float)HEIGHT / 2.0f) - y));

	float sensitivity = 0.10f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	yaw = glm::mod(yaw + xoffset, 360.0f);
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);

	cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));

	if (x != WIDTH / 2 || y != HEIGHT / 2) {
		glutWarpPointer(WIDTH / 2, HEIGHT / 2);
	}
}

//--------------------------------------------------------------------------------
// Rendering
//--------------------------------------------------------------------------------
void Render()
{
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Calculate the deltaTime in seconds
	time = glutGet(GLUT_ELAPSED_TIME);
	deltaTime = (float)((time - oldTime) / 1000.0);
	oldTime = time;

	for (ObjectModel &om : objectModels) 
	{	
		if (om.animationType == AnimationType::noAnimation) {
			//om.model = glm::rotate(om.model, 0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
		}
		if (om.animationType == AnimationType::teaPot) {
			om.model = glm::rotate(om.model, 0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
		}
		if (om.animationType == AnimationType::NPC) {

			float offset = 16.0f;
			stepsize = glm::mod(stepsize, 2.0f * (float)M_PI);
			stepsize += (float)M_PI / 160000.0f;
			animationBuffer = glm::vec3(-animationBuffer.x, -animationBuffer.y, -animationBuffer.z);
			om.model = glm::translate(om.model, animationBuffer);
			animationBuffer = glm::vec3(offset * cos(stepsize), offset * sin(stepsize), 0.0f);
			om.model = glm::rotate(om.model, glm::radians(stepsize), glm::vec3(0.0f, 0.0f, 1.0f));
			om.model = glm::translate(om.model, animationBuffer);
		}
	}

	// Update view.
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    glUseProgram(shaderID);

    for (unsigned int i = 0; i < objectModels.size(); i++)
    {
        objectModels[i].mv = view * objectModels[i].model;

        if (objectModels[i].apply_texture)
        {
            glUniform1i(uniform_apply_texture, 1);
            glBindTexture(GL_TEXTURE_2D, objectModels[i].textureID);
        }
        else
            glUniform1i(uniform_apply_texture, 0);

        glUniformMatrix4fv(uniform_mv, 1, GL_FALSE, glm::value_ptr(objectModels[i].mv));
        glUniform3fv(uniform_material_ambient, 1, glm::value_ptr(objectModels[i].material.ambientColor));
        glUniform3fv(uniform_material_diffuse, 1, glm::value_ptr(objectModels[i].material.diffuseColor));
        glUniform3fv(uniform_material_specular, 1, glm::value_ptr(objectModels[i].material.specular));
        glUniform1f(uniform_material_power, objectModels[i].material.power);

        glBindVertexArray(objectModels[i].vao);
        glDrawArrays(GL_TRIANGLES, 0, objectModels[i].vertices.size());
        glBindVertexArray(0);
    }

    glutSwapBuffers();
}

//------------------------------------------------------------
// void Render(int n)
// Render method that is called by the timer function
//------------------------------------------------------------
void Render(int n)
{
    Render();
    glutTimerFunc(DELTA, Render, 0);
}

//------------------------------------------------------------
// void InitGlutGlew(int argc, char **argv)
// Initializes Glut and Glew
//------------------------------------------------------------
void InitGlutGlew(int argc, char **argv)
{
    glutInit(&argc, argv);

    glutSetOption(GLUT_MULTISAMPLE, 8);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Hello OpenGL");
    glutDisplayFunc(Render);
    glutKeyboardFunc(keyboardHandler);
	glutPassiveMotionFunc(mouseHandler);
    glutTimerFunc(DELTA, Render, 0);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    glewInit();
}

//------------------------------------------------------------
// void InitShaders()
// Initialized the fragmentshader and vertexshader
//------------------------------------------------------------
void InitShaders()
{
    char * fragshader = glsl::readFile(fragshader_name);
    GLuint fshID = glsl::makeFragmentShader(fragshader);

    char * vertexshader = glsl::readFile(vertexshader_name);
    GLuint vshID = glsl::makeVertexShader(vertexshader);

    shaderID = glsl::makeShaderProgram(vshID, fshID);
}

//------------------------------------------------------------
// void InitMatrices()
//------------------------------------------------------------
void InitMatrices()
{
    view = glm::lookAt(
		cameraPos, // Camera position
		(cameraPos + cameraFront), // Camera target
		cameraUp);
    projection = glm::perspective(
        glm::radians(45.0f),
        1.0f * WIDTH / HEIGHT, 
		0.1f,
        400.0f);

	for (unsigned int i = 0; i < objectModels.size(); i++) {
		objectModels[i].mv = view * objectModels[i].model;
	}
}

//------------------------------------------------------------
// void InitObjects()
//------------------------------------------------------------
void InitObjects()
{

	Material material;

	material.ambientColor = glm::vec3(0.0, 0.0, 0.0);
	material.diffuseColor = glm::vec3(0.0, 0.0, 0.0);
	material.specular = glm::vec3(0.1);
	material.power = 128;

	int amountOfPillars = 12;
	// Create adam pillars
	for (double j = 0.0f; j < 2.0f * M_PI; j += 1.0f / (float)(amountOfPillars/2) * M_PI) {
		glm::mat4 modelMatrix = glm::translate(glm::mat4(), glm::vec3(9.5 * cos(j), 0.3, 9.5 * sin(j)));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
		objectModels.push_back(ObjectModel("Textures/steel-cross.bmp", "Objects/amsterdamertje/adam.obj", material, AnimationType::noAnimation, modelMatrix, true));
	}

	// Create floor
	objectModels.push_back(ObjectModel("Textures/grass.bmp", "Objects/floor400X400.obj", material, AnimationType::noAnimation, glm::mat4(), true));

	// Create Sidewalk
	
	glm::mat4 sidewalk = glm::mat4();
	objectModels.push_back(ObjectModel("Textures/sidewalk/sidewalk.bmp", "Objects/Sidewalk.obj", material, AnimationType::noAnimation, sidewalk, true));
	objectModels.push_back(ObjectModel("Textures/rascal.bmp", "Objects/Step.obj", material, AnimationType::noAnimation, sidewalk, true));
	objectModels.push_back(ObjectModel("Textures/sidewalk/roadx10.bmp", "Objects/Road.obj", material, AnimationType::noAnimation, sidewalk, true));

	// Create TeaPotHouse



	Material Window;

	material.ambientColor = glm::vec3(0.0, 0.0, 0.0);
	material.diffuseColor = glm::vec3(0.0, 0.0, 0.0);
	material.specular = glm::vec3(0.1);
	material.power = 128;
	
	glm::mat4 teaPotHouseModel = glm::translate(glm::mat4(), glm::vec3(-23.0f, 0.0f, 2.0f));

	objectModels.push_back(ObjectModel("Textures/TeaPotHouse/StoneWallx3.bmp", "Objects/TeaPotHouse/Base.obj", material, AnimationType::noAnimation, teaPotHouseModel, true));
	objectModels.push_back(ObjectModel("Textures/TeaPotHouse/Glass.bmp", "Objects/TeaPotHouse/Window.obj", material, AnimationType::noAnimation, teaPotHouseModel, true));
	objectModels.push_back(ObjectModel("Textures/TeaPotHouse/TeaPotGoldX3.bmp", "Objects/TeaPotHouse/TeaPot.obj", material, AnimationType::teaPot, teaPotHouseModel, true));
	objectModels.push_back(ObjectModel("Textures/TeaPotHouse/woodTextureX10.bmp", "Objects/TeaPotHouse/BasePorchAndRoof.obj", material, AnimationType::noAnimation, teaPotHouseModel, true));
	objectModels.push_back(ObjectModel("Textures/TeaPotHouse/Door.bmp", "Objects/TeaPotHouse/Door.obj", material, AnimationType::noAnimation, teaPotHouseModel, true));
	objectModels.push_back(ObjectModel("Textures/TeaPotHouse/WoodTexture2.bmp", "Objects/TeaPotHouse/PillarLedge.obj", material, AnimationType::noAnimation, teaPotHouseModel, true));

	// Skybox
	objectModels.push_back(ObjectModel("Textures/Skybox/SkyboxFlipped.bmp", "Objects/Skybox/Skybox.obj", material, AnimationType::noAnimation, glm::mat4(), true));

	// NPC
	glm::mat4 npcModel = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.3f, 0.0f));
	npcModel = glm::rotate(npcModel, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	objectModels.push_back(ObjectModel("Textures/TeaPotHouse/TeaPotGoldX3.bmp", "Objects/KassyaModel/kassya.obj", material, AnimationType::NPC, npcModel, true));
	
}

//------------------------------------------------------------
// void InitMaterialsLight()
//------------------------------------------------------------
void InitMaterialsLight()
{
    light.position = glm::vec3(4.0, 4.0, 4.0);
}

//------------------------------------------------------------
// void InitBuffers()
// Allocates and fills buffers
//------------------------------------------------------------

void InitBuffers()
{
    GLuint vbo_vertices, vbo_normals, vbo_uvs;

    GLuint positionID = glGetAttribLocation(shaderID, "position");
    GLuint normalID = glGetAttribLocation(shaderID, "normal");
    GLuint uvID = glGetAttribLocation(shaderID, "uv");

    // Attach to program (needed to send uniform vars)
    glUseProgram(shaderID);

    // Make uniform vars
    uniform_mv = glGetUniformLocation(shaderID, "mv");
    GLuint uniform_proj = glGetUniformLocation(shaderID, "projection");
    GLuint uniform_light_pos = glGetUniformLocation(shaderID, "lightPos");
    uniform_apply_texture = glGetUniformLocation(shaderID, "applyTexture");
    uniform_material_ambient = glGetUniformLocation(shaderID, "matAmbient");
    uniform_material_diffuse = glGetUniformLocation(shaderID, "matDiffuse");
    uniform_material_specular = glGetUniformLocation(shaderID, "matSpecular");
    uniform_material_power = glGetUniformLocation(shaderID, "matPower");

    // Fill uniform vars
    glUniformMatrix4fv(uniform_proj, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(uniform_light_pos, 1, glm::value_ptr(light.position));

    for (unsigned int i = 0; i < objectModels.size(); i++)
    {
        // vbo for vertices
        glGenBuffers(1, &vbo_vertices);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
        glBufferData(GL_ARRAY_BUFFER, objectModels[i].vertices.size() * sizeof(glm::vec3),
            &objectModels[i].vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // vbo for normals
        glGenBuffers(1, &vbo_normals);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
        glBufferData(GL_ARRAY_BUFFER, objectModels[i].normals.size() * sizeof(glm::vec3),
            &objectModels[i].normals[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // vbo for uvs
        glGenBuffers(1, &vbo_uvs);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_uvs);
        glBufferData(GL_ARRAY_BUFFER, objectModels[i].uvs.size() * sizeof(glm::vec2),
            &objectModels[i].uvs[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenVertexArrays(1, &objectModels[i].vao);
        glBindVertexArray(objectModels[i].vao);

        // Bind vertices to vao
        glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
        glVertexAttribPointer(positionID, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(positionID);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
        glVertexAttribPointer(normalID, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normalID);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_uvs);
        glVertexAttribPointer(uvID, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(uvID);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
    }
}

int main(int argc, char ** argv)
{
    InitGlutGlew(argc, argv);
    InitShaders();
	InitObjects();
    InitMatrices();
    InitMaterialsLight();
    InitBuffers();

    HWND hWnd = GetConsoleWindow();
    //ShowWindow(hWnd, SW_HIDE);
	ShowCursor(false);

    glutMainLoop();

    return 0;
}