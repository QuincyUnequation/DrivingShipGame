#version 330 core

in vec3 TextCoord;
uniform samplerCube  skybox;  // �䨮sampler2D???asamplerCube

out vec4 color;


void main()
{
	color = texture(skybox, TextCoord);
}