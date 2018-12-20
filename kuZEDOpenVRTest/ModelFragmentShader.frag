#version 330 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;    
    float shininess;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 color;

uniform vec3 CamPos;
uniform Material material;
uniform sampler2D ourTexture;
uniform vec4 ObjColor;


void main()
{    
	vec3 LightColor = vec3(1.0, 1.0, 1.0);
	vec3 LightPos   = CamPos;
	vec3 viewPos    = CamPos;

	// Ambient
	vec3 ambient = LightColor * material.ambient;

	// Diffuse
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(LightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * LightColor * material.diffuse;

	// Specular
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 1.0);
	vec3 specular = spec * LightColor * material.specular;
	//vec3 specular = vec3(0.0, 0.0, 0.0);

				   // lighting color					      // object color
	//color = vec4(ambient + diffuse + specular, 1.0f) * vec4(1.0f, 1.0f, 1.0f, 1.0f);
	//color = vec4(ambient + diffuse + specular, 1.0f) * texture(ourTexture, TexCoord);
	color = vec4(ambient + diffuse + specular, 1.0f) * ObjColor;
}