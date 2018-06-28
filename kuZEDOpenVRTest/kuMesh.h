#ifndef KU_MESH_H
#define KU_MESH_H

#pragma once

#include <string>
#include <vector>
#include <GLEW/glew.h>
#include <GLM/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "kuShaderHandler.h"

using namespace std;

struct kuVertex {
	glm::vec3		Position;
	glm::vec3		Normal;
	glm::vec2		TexCoord;
};

struct kuTexture {
	GLuint	 TextureID;
	string   type;
};

struct kuMaterial {
	glm::vec3		Ambient;
	glm::vec3		Diffuse;
	glm::vec3		Specular;
};

class kuMesh
{
public:
	vector<kuVertex>	vertices;
	vector<GLuint>		indices;
	vector<kuTexture>	textures;

	kuMesh(vector<kuVertex> vertices, vector<GLuint> indices, vector<kuTexture> textures);
	void Draw(kuShaderHandler shader);

	kuMesh();
	~kuMesh();
private:
	GLuint	VAO, VBO, EBO;
	
	void	setupMesh();
};

#endif // !KU_MESH_H
