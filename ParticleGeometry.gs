#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

out vec4 viewSpacePos;

uniform mat4 projection;

void createBillBoard(vec4 position) 
{
	 float size = 1.0;

	viewSpacePos = position + vec4(-size, -size, size * 0.5, 0.0);
	gl_Position = projection * viewSpacePos;
	EmitVertex(); 

	viewSpacePos = position + vec4(size, -size, size * 0.5, 0.0); 
	gl_Position = projection * viewSpacePos;
    EmitVertex();

	viewSpacePos = position + vec4(-size, size, size * 0.5, 0.0); 
	gl_Position = projection * viewSpacePos;
    EmitVertex();

	viewSpacePos =  position + vec4(size, size, size * 0.5, 0.0); 
	gl_Position = projection * viewSpacePos;
    EmitVertex();

	EndPrimitive();
}

void main() {
	// gl_Position is in View Space Coordinates
	createBillBoard(gl_in[0].gl_Position);
} 

