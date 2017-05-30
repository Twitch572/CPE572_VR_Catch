#version 330 core

layout(location = 0, rowmajor) in vec3 vertPos;

void main() {
	gl_Position = vec4(vertPos, 1.0);
}
