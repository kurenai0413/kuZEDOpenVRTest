#include "kuMesh.h"

kuMesh::kuMesh(vector<kuVertex> vertices, vector<GLuint> indices, vector<kuTexture> textures)
{
	this->vertices = vertices;
	this->indices  = indices;
	this->textures = textures;

	this->setupMesh();
}

void kuMesh::Draw(kuShaderHandler shader)
{
	GLuint	diffuseNr  = 1;
	GLuint  specularNr = 1;

	for (int i = 0; i < this->textures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		stringstream ss;
		string number;
		string name = this->textures[i].type;
		if (name == "texture_diffuse")
			ss << diffuseNr++; // Transfer GLuint to stream
		else if (name == "texture_specular")
			ss << specularNr++; // Transfer GLuint to stream
		number = ss.str();

		glUniform1f(glGetUniformLocation(shader.ShaderProgramID, ( name + number).c_str()), i);
		glBindTexture(GL_TEXTURE_2D, this->textures[i].TextureID);	
	}
	glActiveTexture(GL_TEXTURE0);

	glBindVertexArray(this->VAO);
	glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	for (GLuint i = 0; i < this->textures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

kuMesh::kuMesh()
{
}


kuMesh::~kuMesh()
{
}

void kuMesh::setupMesh()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(this->VAO);
	
	glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
	glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(kuVertex), 
				 &this->vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint),
				 &this->indices[0], GL_STATIC_DRAW);

	// position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(kuVertex), (GLvoid *)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(kuVertex), (GLvoid *)offsetof(kuVertex, Normal));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(kuVertex), (GLvoid *)offsetof(kuVertex, TexCoord));

	glBindVertexArray(0);
}
