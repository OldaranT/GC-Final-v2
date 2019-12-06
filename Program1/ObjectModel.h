#pragma once
#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "glsl.h"

enum AnimationType { noAnimation, teaPot, NPC };

struct Material
{
	glm::vec3 ambientColor;
	glm::vec3 diffuseColor;
	glm::vec3 specular;
	float power;
};
class ObjectModel
{
public:

	GLuint vao;

	glm::mat4 model; 
	AnimationType animationType;

	GLuint textureID;
	glm::mat4 mv;
	Material material;
	bool apply_texture;

	vector<glm::vec3> vertices;
	vector<glm::vec3> normals;
	vector<glm::vec2> uvs;

	ObjectModel(const char* matPath, const char* objPath, Material mat, AnimationType animationType, glm::mat4 modelMatrix, bool applyTexture);
	~ObjectModel();
};

