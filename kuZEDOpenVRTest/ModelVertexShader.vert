#version 410 core

uniform mat4 matrix;
uniform mat4 ModelMat;
uniform mat4 ViewMat;
uniform mat4 ProjMat;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

//out vec3 ourColor;
out vec3 Normal;
out vec3 FragPos;

void main()
{
	gl_Position = ProjMat *  matrix /** ViewMat*/ * ModelMat * vec4(position, 1.0);
	//ourColor = vec3(1.0, 1.0, 1.0);

	//FragPos = position;
	//Normal  = normal;

	FragPos = vec3(ModelMat * vec4(position, 1.0f));
	Normal = mat3(transpose(inverse(ModelMat))) * normal;

	//Normal = vec3(-Normal.x, -Normal.y, -Normal.z);
}