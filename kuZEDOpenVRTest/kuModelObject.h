#ifndef KU_MODELOBJECT_H
#define KU_MODELOBJECT_H

#pragma once

#include <vector>
#include <GLEW/glew.h>
#include <GLM/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "kuMesh.h"
#include "kuShaderHandler.h"

using namespace std;

class kuModelObject
{
public:

	kuModelObject(char * filename);
	kuModelObject(char * filename, kuShaderHandler shader);
	kuModelObject();
	~kuModelObject();

	void Draw(kuShaderHandler shader);
	void Draw();
	void Draw(kuShaderHandler shader, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular);
	void Draw(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular);
	void Draw(kuShaderHandler shader, kuMaterial material);
	void Draw(kuMaterial material);
	
	void SetMaterial(kuMaterial material);

private:
	kuShaderHandler		m_Shader;

	vector<kuMesh>		m_ObjectMeshes;
	vector<kuMaterial>	m_ObjectMaterials;
	vector<kuTexture>	m_ObjectTexture;

	void LoadModel(char * filename);
	void ProcessNode(aiNode * node, const aiScene * scene);
	kuMesh processMesh(aiMesh* mesh, const aiScene* scene);
	vector<kuTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
										   string typeName);
};

#endif // !KU_MODELOBJECT_H