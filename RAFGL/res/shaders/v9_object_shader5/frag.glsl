#version 330

in vec3 pass_normal;
in vec3 pass_world_position;

out vec4 final_colour;

uniform vec3 uni_object_colour;
uniform vec3 uni_light_colour;
uniform vec3 uni_light_direction;
uniform vec3 uni_ambient;
uniform vec3 uni_camera_position;

uniform samplerCube skybox_tex;

float random(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

float randomc(vec2 st)
{
	float r = random(st);
	r = r * 2.0 - 1.0;
	r = r * 0.1;
	return r;
}

void main()
{
	float decimal = 400;
	vec3 view_vector = normalize(pass_world_position - uni_camera_position);
	vec3 normalized_normal = normalize(pass_normal);
	
	vec3 reflected_view_vector = reflect(view_vector, normalized_normal);
	
	float refraction_ratio = 1.00 / 1.309;
	vec3 refracted_view_vector = refract(view_vector, normalized_normal, refraction_ratio);
	
	float fresnel = dot((reflected_view_vector * -1), view_vector);
    fresnel = clamp(1.0 - fresnel, 0.0, 1.0);
    fresnel = pow(fresnel, 5.0);
	
	vec3 reflected_colour = texture(skybox_tex, reflected_view_vector).rgb;
	reflected_colour = mix(vec3(0.0), reflected_colour, 0.8);
	
	vec3 refracted_colour = texture(skybox_tex, refracted_view_vector).rgb;
	refracted_colour = mix(vec3(0.0), refracted_colour, 0.8);
	
	final_colour = vec4(mix(refracted_colour, reflected_colour, fresnel) * uni_object_colour, 1.0);
	
}