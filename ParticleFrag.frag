#version 330 core
out vec4 FragColor;

in vec4 viewSpacePos; 

uniform vec3[10] neighboringParticles;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 lightPos;

uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;

//used to switch between the two scalarfield functions
uniform bool linearMode;

float computeScalarField(vec3 point) 
{
	float scalarField = 0;
	float maxI = 2.0;

	float R = 1.0;

	for(int i = 0; i < 10; ++i) 
	{
	float d = distance(point, neighboringParticles[i]);
		if(true == linearMode) {
			if(d < R) {
				scalarField += R - d;
			}
		}
		else {
			float exponent = 3.0f * d;
			scalarField += 1.0f / exp(exponent);
		}	
	}
	return scalarField;
}

vec3 computeGradientVector(vec3 point) 
{
	float nudge = 0.01;
	vec3 computedVector = vec3(
	(computeScalarField(vec3(point.x + nudge, point.y, point.z)) - computeScalarField(vec3(point.x - nudge, point.y, point.z))),
	(computeScalarField(vec3(point.x, point.y + nudge, point.z)) - computeScalarField(vec3(point.x, point.y - nudge, point.z))),
	(computeScalarField(vec3(point.x, point.y, point.z + nudge)) - computeScalarField(vec3(point.x, point.y, point.z - nudge)))
	);
	return (1.0f / (2.0f * nudge)) * computedVector;
}

void main()
{	
	vec3 connectionVector = normalize(viewSpacePos.xyz);
	int alpha = 0;
	float scalarField;
	vec3 start = vec3(0);
	vec3 dt = 0.05 * connectionVector;

	//sampling the view vector for a fixed number of times and determine whether we hit the scalar field
	for(int z = 0; z < 200; ++z) 
	{	
		start += dt;
		scalarField = computeScalarField(start);

		if(scalarField > 0.5) {
			alpha = 1;
			dt = 0.005 * connectionVector;
			
			for(int i = 0; i < 10; ++i) {
				start -= dt;
				scalarField = computeScalarField(start);
				dt *= 0.5;

				if(scalarField > 0.5) {
					start -= dt;
				}
				else {
					start += dt;
				}
			}
			break;
		}
	}
	//discard alone can lead to undefined behaviour on some GPUs hence the return is added
	if(alpha == 0) {
		discard;
		return;
	}
	
	//classic specular lighting
	vec3 gradientNormal = normalize(computeGradientVector(start));
	
	vec3 lightDir = normalize(-viewSpacePos.xyz);
	vec3 viewDir = normalize(-viewSpacePos.xyz);
	vec3 reflectDir = reflect(-lightDir, gradientNormal);

	float diff = max(dot(gradientNormal, lightDir), 0.0);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
	vec3 specular = specularStrength * spec * lightColor;
	vec3 diffuse = diff * lightColor;

	vec3 ambient = ambientStrength * lightColor;
	
	vec3 result = (ambient + diffuse + specular) * objectColor;

	FragColor = vec4(result, 1.0);	
} 