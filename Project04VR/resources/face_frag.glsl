#version 330 core 
in vec3 fragNor;
out vec4 color;

void main()
{
	vec3 normal = normalize(fragNor);
	color = vec4(0.0, 0.0, 0.0, 1.0);
}
