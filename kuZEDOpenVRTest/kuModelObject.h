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
	kuModelObject();
	~kuModelObject();

	void Draw(kuShaderHandler shader);

private:
	vector<kuMesh>		ObjectMeshes;
	vector<kuMaterial>	ObjectMaterials;
	vector<kuTexture>	TextureLoaded;

	void LoadModel(char * filename);
	void ProcessNode(aiNode * node, const aiScene * scene);
	kuMesh processMesh(aiMesh* mesh, const aiScene* scene);
	vector<kuTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
										   string typeName);
};

#endif // !KU_MODELOBJECT_H