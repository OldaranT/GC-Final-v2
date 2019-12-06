#include "ObjectModel.h"
#include <string>
#include "objloader.hpp"
#include "texture.hpp"



ObjectModel::ObjectModel(const char* matPath, const char* objPath, Material mat, AnimationType animType, glm::mat4 modelMatrix = glm::mat4(), bool applyTexture = true)
{
	model = modelMatrix;
	material = mat;
	animationType = animType;
	apply_texture = applyTexture;
	loadOBJ(objPath, vertices, uvs, normals); 
	textureID = loadBMP(matPath);
}


ObjectModel::~ObjectModel()
{
}
