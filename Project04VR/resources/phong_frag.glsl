#version 330 core 

in vec3 fragNor;
in vec3 pos;
uniform mat4 M;
uniform mat4 V;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float shine;
out vec4 color;

void main()
{
	vec3 normal = normalize(fragNor);
	vec3 view = normalize(-1 * pos);
	vec3 curLightPos = normalize(lightPos - pos);

	vec3 R = -curLightPos + 2 * dot(curLightPos, normal) * normal;
	vec3 spec = MatSpec * pow(max(0.0f, dot(R, view)), shine) * lightColor;
	vec3 diff = MatDif * max(0.0f, dot(normal, curLightPos)) * lightColor;
	vec3 amb = MatAmb * lightColor;
	vec3 fragColor = spec + diff + amb;
	color = vec4(fragColor, 1.0f);
}
