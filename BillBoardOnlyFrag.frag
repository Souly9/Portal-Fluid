#version 330 core
out vec4 FragColor;

in vec4 viewSpacePos;

void main()
{	
	FragColor = vec4(viewSpacePos);
} 