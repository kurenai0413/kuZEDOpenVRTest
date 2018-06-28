#include "kuModelObject.h"



kuModelObject::kuModelObject(char * filename)
{
	this->LoadModel(filename);
}

kuModelObject::kuModelObject()
{
}


kuModelObject::~kuModelObject()
{
}

void kuModelObject::Draw(kuShaderHandler shader)
{
	glUniform3f(glGetUniformLocation(shader.ShaderProgramID, "material.ambient"),
				ObjectMaterials[0].Ambient.r, ObjectMaterials[0].Ambient.g, ObjectMaterials[0].Ambient.b);
	glUniform3f(glGetUniformLocation(shader.ShaderProgramID, "material.diffuse"),
				ObjectMaterials[0].Diffuse.r, ObjectMaterials[0].Diffuse.g, ObjectMaterials[0].Diffuse.b);
	glUniform3f(glGetUniformLocation(shader.ShaderProgramID, "material.specular"),
				ObjectMaterials[0].Specular.r, ObjectMaterials[0].Specular.g, ObjectMaterials[0].Specular.b);

	for (int i = 0; i < ObjectMeshes.size(); i++)
	{
		this->ObjectMeshes[i].Draw(shader);
	}
}

void kuModelObject::LoadModel(char * filename)
{
	Assimp::Importer	importer;

	cout << "Loading model....." << filename << endl;

	const aiScene * scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | 
														aiProcess_GenNormals );
	// aiProcessPreset_TargetRealtime_MaxQuality 可以試試看
	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		cout << "ERROR::ASSIMP::" << importer.GetErrorString() << endl;
		return;
	}

	bool hasAnimation = scene->HasAnimations();
	bool hasCamera = scene->HasCameras();
	bool hasLight = scene->HasLights();
	bool hasMaterial = scene->HasMaterials();
	bool hasMeshes = scene->HasMeshes();
	bool hasTextures = scene->HasTextures();

	// Process ASSIMP's root node recursively
	this->ProcessNode(scene->mRootNode, scene);

	cout << "Done." << endl;
}

// 根據node從scene的meshes裡挖東西
// 照這樣來看Assimp的mesh是全部存在aiScene的member mMeshes裡(aiMesh陣列)
// aiNode裡面的member mMeshes只存index而已
void kuModelObject::ProcessNode(aiNode * node, const aiScene * scene)
{
	for (int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh * mesh = scene->mMeshes[node->mMeshes[i]];			// 去從scene的mMeshes裡面根據node裡存的index要mesh出來

		this->ObjectMeshes.push_back(this->processMesh(mesh, scene));
	}

	for (int i = 0; i < node->mNumChildren; i++)
	{
		this->ProcessNode(node->mChildren[i], scene);
	}
}

kuMesh kuModelObject::processMesh(aiMesh * mesh, const aiScene * scene)
{
	vector<kuVertex>		vertices;
	vector<GLuint>			indices;
	vector<kuTexture>		textures;

#pragma region // Fill vertices // 
	// vertices完全靠vertex的數量決定(喂，廢話)
	for (int i = 0; i < mesh->mNumVertices; i++)
	{
		kuVertex	Vert;
		glm::vec3 VertPos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		glm::vec3 VertNormal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
		glm::vec2 VertTexCoord;
		// Assimp allows a model to have up to 8 different texture coordinates per vertex
		if (mesh->mTextureCoords[0])				// Does the mesh contain texture coordinates?
		{
			VertTexCoord.x = mesh->mTextureCoords[0][i].x;
			VertTexCoord.y = mesh->mTextureCoords[0][i].y;
		}
		else
		{
			VertTexCoord.x = 0.0f;
			VertTexCoord.y = 0.0f;
		}
		
		Vert.Position = VertPos;
		Vert.Normal   = VertNormal;
		Vert.TexCoord = VertTexCoord;

		vertices.push_back(Vert);
	}
#pragma endregion

#pragma region // Fill face indices //
	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];			// 從mesh內取出faces
		for (int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}
#pragma endregion

	// 嗯 看不懂 改天k完lighting再說
	aiColor4D diffuse;
	aiColor4D specular;
	aiColor4D ambient;
	kuMaterial materialTemp;

	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		
		aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse);
		aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &specular);
		aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &ambient);

		materialTemp.Ambient  = glm::vec3(ambient.r, ambient.g, ambient.b);
		materialTemp.Diffuse  = glm::vec3(diffuse.r, diffuse.g, diffuse.b);
		materialTemp.Specular = glm::vec3(specular.r, specular.g, specular.b);

		ObjectMaterials.push_back(materialTemp);
	}

	return kuMesh(vertices, indices, textures);
}

vector<kuTexture> kuModelObject::loadMaterialTextures(aiMaterial * mat, aiTextureType type, string typeName)
{
	// 沒texture的關係，感覺可以先忽略
	vector<kuTexture> textures;
	
	for (GLuint i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		/*
		GLboolean skip = false;
		for (GLuint j = 0; j < TextureLoaded.size(); j++)
		{
			if (TextureLoaded[j].path == str)
			{
				textures.push_back(textures_loaded[j]);
				skip = true;
				break;
			}
		}
		if (!skip)
		{   // If texture hasn't been loaded already, load it
			kuTexture texture;
			texture.id = TextureFromFile(str.C_Str(), this->directory);
			texture.type = typeName;
			texture.path = str;
			textures.push_back(texture);
			this->textures_loaded.push_back(texture);  // Add to loaded textures
		}
		*/
	}
	
	return textures;
}
