#version 410 core
in vec3 normal;
in vec4 fragPosEye;
in vec2 textureCoordonates;
out vec4 fColor;
uniform mat3 normalMatrix;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

vec3 ambient;
float ambientStrength=0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength=0.5f;
float shininess=32.0f;
float constant=1.0f;
float linear=0.0045f;
float quadratic=0.0075f;

void computeLight(){
	vec3 cameraPosEye=vec3(0.0f);
	vec3 normalEye=normalize(normalMatrix*normal);

	vec3 lightDirN=normalize(lightDir);
	vec3 viewDirN=normalize(cameraPosEye-fragPos.xyz);
	float dist=length(lightDir-fragPosEye.xyz);
	float att=1.0/(constant+linear*dist+quadratic*(dist*dist));
	ambient=att*ambientStrength*lightColor;
	diffuse=att*max(dot(normalEye,lightDirN),0.0f)*lightColor;
	vec3 halfVector=normalize(lightDirN+viewDirN);
	vec3 reflection=reflect(-lightDirN,normalEye);
	float specCoeff=pow(max(dot(normalEye,helfVector),0.0f),shininess);
	specular=att*specularStrength*specCoeff*lightColor;
}
int main(){
	computeLight();
	vec3 basecColor=vec3(1.0f,0.55f,0.0f);
	ambient*=texture(diffuseTexture,textureCoordonates);
	diffuse*=texture(diffuseTexture,textureCoordonates);
	specular*=texture(specularTexture,textureCoordonates);
	vec3 color=min((ambient+diffuse)+specular,1.0f);
	fColor=vec4(color,1.0f);
}