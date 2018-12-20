#include "kuShaderHandler.h"



kuShaderHandler::kuShaderHandler()
{
}

kuShaderHandler::kuShaderHandler(const char * VSPathName, const char * FSPathName)
{
	this->Load(VSPathName, FSPathName);
}

kuShaderHandler::~kuShaderHandler()
{

}

bool kuShaderHandler::Load(const char * VSPathName, const char * FSPathName)
{
	//#pragma region
	GLuint vertexShader, fragmentShader;
	GLint  success;
	GLchar infoLog[512];

	bool res = CreateShader(vertexShader, ShaderType::VERTEX, VSPathName);
	res = CreateShader(fragmentShader, ShaderType::FRAGMENT, FSPathName);

	// Shader Program
	this->m_ShaderProgramID = glCreateProgram();
	glAttachShader(this->m_ShaderProgramID, vertexShader);
	glAttachShader(this->m_ShaderProgramID, fragmentShader);
	glLinkProgram(this->m_ShaderProgramID);
	// Print linking errors if any
	glGetProgramiv(this->m_ShaderProgramID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(this->m_ShaderProgramID, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		m_fShaderCreated = false;
	}
	else
	{
		// Delete the shaders as they're linked into our program now and no longer necessery
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		m_fShaderCreated = true;
	}

	return m_fShaderCreated;
}

bool kuShaderHandler::Use()
{
	if (m_fShaderCreated)
	{
		glUseProgram(this->m_ShaderProgramID);
		return true;
	}
	else
	{
		return false;
	}
}

GLuint kuShaderHandler::GetShaderProgramID()
{
	return this->m_ShaderProgramID;
}

bool kuShaderHandler::CreateShader(GLuint & shaderID, ShaderType shaderType, const char * shaderPathName)
{
	std::string		shaderCode;
	std::ifstream	shaderFile;

	shaderFile.exceptions(std::ifstream::badbit);

	try
	{
		shaderFile.open(shaderPathName);

		if (shaderFile.is_open())
		{
			std::stringstream shaderStream;
			shaderStream << shaderFile.rdbuf();

			shaderFile.close();

			shaderCode = shaderStream.str();
		}
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}

	const GLchar * shaderCodeArray = shaderCode.c_str();

#pragma region // Compile shaders
	GLint  success;
	GLchar infoLog[512];

	switch (shaderType)
	{
	case ShaderType::VERTEX:
		shaderID = glCreateShader(GL_VERTEX_SHADER);
		break;
	case ShaderType::FRAGMENT:
		shaderID = glCreateShader(GL_FRAGMENT_SHADER);
		break;
	default:
		break;
	}

	bool res = CompileShader(shaderID, shaderCodeArray);

	if (res)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool kuShaderHandler::CompileShader(GLuint shaderID, const GLchar * shaderCode)
{
	GLint  compileStatus;
	GLchar infoLog[512];

	glShaderSource(shaderID, 1, &shaderCode, NULL);
	glCompileShader(shaderID);
	// Print compile errors if any
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);
	if (!compileStatus)
	{
		glGetShaderInfoLog(shaderID, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		return false;
	}
	else
	{
		return true;
	}
}
