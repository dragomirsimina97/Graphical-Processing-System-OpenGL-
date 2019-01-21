#version 410 core

in vec3 normal;
in vec4 fragPosEye;
in vec2 textureCoordonates;
in vec4 fragPosLightSpace;
out vec4 fColor;

//lighting
uniform	mat3 normalMatrix;
uniform mat3 lightDirMatrix;
uniform	vec3 lightDir;
uniform	vec3 lightColor;
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;
vec3 ambient;
float ambientStrength = 0.5f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 10;
float constant=1.0f;
float linear=0.0045f;
float quadratic=0.0075f;




struct DirLight {
    vec3 direction;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};



#define NR_POINT_LIGHTS 9

in vec3 FragPos;
out vec4 FragColor;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform float distanceFog;




void computeLightComponents()
{		
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 normalEye = normalize(normalMatrix * normal);	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDirMatrix*lightDir);
	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);
		
	float dist=length(lightDir-fragPosEye.xyz);
	float att=1.0/(constant+linear*dist+quadratic*(dist*dist));//lumina punctiforma
	//compute ambient light
	//ambient = att*ambientStrength * lightColor;
	ambient=ambientStrength*lightColor;
	//compute diffuse light
	//diffuse = att*max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	diffuse=max(dot(normalEye,lightDirN),0.0f)*lightColor;
	//normalize light direction
	
	
	
	//compute view direction
	
	
	//compute hal vector
	
	vec3 halfVector=normalize(lightDirN+viewDirN);
	
	
	//compute specular light
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
	//specular = att*specularStrength * specCoeff * lightColor;
	specular = specularStrength*specCoeff*lightColor;
}
float computeShadow(){
	vec3 normalizeCoords=fragPosLightSpace.xyz/fragPosLightSpace.w;
	normalizeCoords=normalizeCoords*0.5+0.5;

	if (normalizeCoords.z >= 1.0f)
		return 0.0f;

	float closestDepth=texture(shadowMap,normalizeCoords.xy).r;
	float curentDepth=normalizeCoords.z;

	float biass=0.005f;
	float shadow=curentDepth-biass>closestDepth?1.0f:0.0f;
//	return 1.0f;
	return shadow;

}

// calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
	float shadow=computeShadow();
    vec3 lightDirN = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDirN), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDirN, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    // combine results
    vec3 ambient = light.ambient * vec3(texture(diffuseTexture, textureCoordonates));
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuseTexture, textureCoordonates));
    vec3 specular = light.specular * spec * vec3(texture(specularTexture, textureCoordonates));
    return min((ambient + (1.0f - shadow)*diffuse) + (1.0f - shadow)*specular, 1.0f);
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	
    vec3 lightDirN = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDirN), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDirN, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient = light.ambient * vec3(texture(diffuseTexture, textureCoordonates));
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuseTexture, textureCoordonates));
    vec3 specular = light.specular * spec * vec3(texture(specularTexture, textureCoordonates));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

// calculates the color when using a spot light.




float computeFog(){
    //float distanceFog=0.5f;
   // float distanceFog=0.0f;
    float fragmentFog=length(fragPosEye);
    float fog=exp(-pow(fragmentFog*distanceFog,2));

    return clamp(fog,0.0,1.0);
}
    
void main() 
{
	//computeLightComponents();
	
    float shadow = computeShadow();
    float fog = computeFog();
    vec4 fogColor = vec4(0.5, 0.5, 0.5, 1.0);
    
	vec3 baseColor = vec3(1.0f, 0.55f, 0.0f);//orange
	//vec3 t=vec3(vTexCoords,1.0f);
	ambient *= vec3(texture(diffuseTexture,textureCoordonates));
	diffuse *= vec3(texture(diffuseTexture,textureCoordonates));
	specular *= vec3(texture(specularTexture,textureCoordonates));
	vec3 color = min((ambient + (1.0f - shadow)*diffuse) + (1.0f - shadow)*specular, 1.0f);
    // properties
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(-FragPos);

    // phase 1: directional lighting
    vec3 result = CalcDirLight(dirLight, norm, viewDir);//culoare
    // phase 2: point lights
   for(int i = 0; i < NR_POINT_LIGHTS; i++)
       result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);    
    // phase 3: spot light
    
    
    FragColor =fogColor * (1-fog)+ vec4(result, 1.0)*fog;
	
	//ambient*=baseColor;
	//diffuse*=baseColor;
	//specular*=baseColor;
	
    
	//fColor = fogColor * (1-fog) + vec4(color,1.0) * fog;
	//fColor=vec4(result,1.0f);
}
