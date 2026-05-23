#version 330 core
layout(location = 0) in vec3 vertex;
in vec4 col;


uniform	 vec3 center;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform mat4 V;
uniform mat4 P;


out vec2 uv;
uniform float healthPercent;
out vec4 color;


void main(){
	//center is x,y,z pos of center + w for the size of the particle
	float scaleX = 0.2;
	float scaleY = 1.0 * healthPercent; 
	float health = clamp(healthPercent, 0.0, 1.0);
	vec4 redColor = vec4(1.0, 0.0, 0.0, 1.0);
	vec4 greenColor = vec4(0.0, 1.0, 0.0, 1.0);
	color = mix(redColor, greenColor, health);

	vec3 particleCenter = center;

	//replace the usage of model matrice with just a translation for the position 
	vec3 vertexPosition_worldspace = particleCenter + cameraRight * vertex.x * scaleX + cameraUp * vertex.y * scaleY;
	

	gl_Position = P*V*vec4(vertexPosition_worldspace, 1.0);
	//gl_Position /= gl_Position.w;
}