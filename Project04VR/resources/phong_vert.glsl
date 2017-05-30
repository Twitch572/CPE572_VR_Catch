#version  330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightPos;
uniform vec3 lightColor;
out vec3 fragNor;
out vec3 pos;

void main()
{
	gl_Position = P * V * M * vertPos;
	pos = vec3(V * M * vertPos);
	fragNor = (V * M * vec4(vertNor, 0.0)).xyz;
}
