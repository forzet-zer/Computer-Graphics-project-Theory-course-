#version 330 core

#define NUMBER_OF_POINT_LIGHTS 4

struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirLight
{
    vec3 direction;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight
{
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NUMBER_OF_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;
uniform int transparency;

// Fresnel (solo para el mar): reflejo del cielo que aumenta en angulos rasantes
uniform int fresnel;            // 0 = Phong normal, 1 = agua con Fresnel
uniform sampler2D skyReflect;   // textura equirectangular del cielo (unidad 1)
uniform float time;             // segundos, para mover las ondas
const float PI = 3.14159265;

// Function prototypes
vec3 CalcDirLight( DirLight light, vec3 normal, vec3 viewDir );
vec3 CalcPointLight( PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir );
vec3 CalcSpotLight( SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir );

void main( )
{
    // Properties
    vec3 norm = normalize( Normal );
    vec3 viewDir = normalize( viewPos - FragPos );

    if ( fresnel == 1 )
    {
        // Ondas: se inclina la normal con senos que avanzan en el tiempo (el plano no se deforma,
        // solo cambia como refleja la luz y el cielo)
        vec2 p = FragPos.xz;
        float dx = 0.06 * cos( p.x * 0.35 + time * 1.3 ) + 0.03 * cos( ( p.x + p.y ) * 0.9 + time * 2.1 );
        float dz = 0.06 * cos( p.y * 0.28 + time * 1.1 ) + 0.03 * cos( ( p.x - p.y ) * 0.7 + time * 1.7 );
        // A lo lejos las ondas se ven de menos de un pixel y generan aliasing: se atenuan con la distancia
        float atenuacion = clamp( 1.0 - length( viewPos - FragPos ) / 60.0, 0.0, 1.0 );
        norm = normalize( vec3( -dx * atenuacion, 1.0, -dz * atenuacion ) );
    }
    
    // Directional lighting
    vec3 result = CalcDirLight( dirLight, norm, viewDir );
    
    // Point lights
    for ( int i = 0; i < NUMBER_OF_POINT_LIGHTS; i++ )
    {
        result += CalcPointLight( pointLights[i], norm, FragPos, viewDir );
    }
    
    // Spot light
    result += CalcSpotLight( spotLight, norm, FragPos, viewDir );
 	
    if ( fresnel == 1 )
    {
        // Aproximacion de Schlick: F = F0 + (1 - F0) * (1 - cos(theta))^5, con F0 = 0.02 para el agua
        float cosTheta = max( dot( norm, viewDir ), 0.0 );
        float F = 0.02 + 0.98 * pow( 1.0 - cosTheta, 5.0 );

        // Reflejo del cielo: direccion reflejada -> coordenadas equirectangulares de la textura
        vec3 R = reflect( -viewDir, norm );
        R.y = abs( R.y );   // el agua solo refleja el cielo (hemisferio de arriba)
        vec2 uvCielo = vec2( atan( R.z, R.x ) / ( 2.0 * PI ) + 0.5, acos( clamp( R.y, -1.0, 1.0 ) ) / PI );
        vec3 cielo = texture( skyReflect, uvCielo ).rgb * dirLight.ambient;   // se oscurece de noche

        result = mix( result, cielo, F );
    }

    color = vec4( result,texture(material.diffuse, TexCoords).a );
	  if(color.a < 0.1 && transparency==1)
        discard;

}

// Calculates the color when using a directional light.
vec3 CalcDirLight( DirLight light, vec3 normal, vec3 viewDir )
{
    vec3 lightDir = normalize( -light.direction );
    
    // Diffuse shading
    float diff = max( dot( normal, lightDir ), 0.0 );
    
    // Specular shading
    vec3 reflectDir = reflect( -lightDir, normal );
    float spec = pow( max( dot( viewDir, reflectDir ), 0.0 ), material.shininess );
    
    // Combine results
    vec3 ambient = light.ambient * vec3( texture( material.diffuse, TexCoords ) );
    vec3 diffuse = light.diffuse * diff * vec3( texture( material.diffuse, TexCoords ) );
    vec3 specular = light.specular * spec * vec3( texture( material.specular, TexCoords ) );
    
    return ( ambient + diffuse + specular );
}

// Calculates the color when using a point light.
vec3 CalcPointLight( PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir )
{
    vec3 lightDir = normalize( light.position - fragPos );
    
    // Diffuse shading
    float diff = max( dot( normal, lightDir ), 0.0 );
    
    // Specular shading
    vec3 reflectDir = reflect( -lightDir, normal );
    float spec = pow( max( dot( viewDir, reflectDir ), 0.0 ), material.shininess );
    
    // Attenuation
    float distance = length( light.position - fragPos );
    float attenuation = 1.0f / ( light.constant + light.linear * distance + light.quadratic * ( distance * distance ) );
    
    // Combine results
    vec3 ambient = light.ambient * vec3( texture( material.diffuse, TexCoords ) );
    vec3 diffuse = light.diffuse * diff * vec3( texture( material.diffuse, TexCoords ) );
    vec3 specular = light.specular * spec * vec3( texture( material.specular, TexCoords ) );
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return ( ambient + diffuse + specular );
}

// Calculates the color when using a spot light.
vec3 CalcSpotLight( SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir )
{
    vec3 lightDir = normalize( light.position - fragPos );
    
    // Diffuse shading
    float diff = max( dot( normal, lightDir ), 0.0 );
    
    // Specular shading
    vec3 reflectDir = reflect( -lightDir, normal );
    float spec = pow( max( dot( viewDir, reflectDir ), 0.0 ), material.shininess );
    
    // Attenuation
    float distance = length( light.position - fragPos );
    float attenuation = 1.0f / ( light.constant + light.linear * distance + light.quadratic * ( distance * distance ) );
    
    // Spotlight intensity
    float theta = dot( lightDir, normalize( -light.direction ) );
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp( ( theta - light.outerCutOff ) / epsilon, 0.0, 1.0 );
    
    // Combine results
    vec3 ambient = light.ambient * vec3( texture( material.diffuse, TexCoords ) );
    vec3 diffuse = light.diffuse * diff * vec3( texture( material.diffuse, TexCoords ) );
    vec3 specular = light.specular * spec * vec3( texture( material.specular, TexCoords ) );
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return ( ambient + diffuse + specular );
}