#version 330

in vec3 pass_normal;
in vec3 pass_world_position;
in vec2 pass_uv;

out vec4 final_colour;

uniform sampler2D albedo;

uniform vec3 uni_light_colour;
uniform vec3 uni_light_direction;
uniform vec3 uni_camera_position;

uniform float v_offset;
uniform float v_scale;

const float specular_limit = 0.0;

float brightness(vec3 pix)
{
	return (pix.r + pix.g + pix.b) / 3.0;
}

void main()
{

	vec2 uv = pass_uv * vec2(1.0, v_scale).yx + vec2(0.0, v_offset).yx;
	vec3 ambient = vec3(0.2, 0.2, 0.2);
	vec3 object_colour = texture(albedo, uv).rgb;

	vec3 view_vector = normalize(pass_world_position - uni_camera_position);
	vec3 neg_view_vector = view_vector * (-1);
	vec3 normalized_normal = normalize(pass_normal);
	vec3 reflected_light = reflect(normalize(uni_light_direction), normalized_normal);
	
	float specular_factor = clamp(dot(reflected_light, neg_view_vector), 0, 1);
	specular_factor = pow(specular_factor * specular_limit, 5.0);
	vec3 specular_colour = object_colour * uni_light_colour * specular_factor;
	
	float light_factor = clamp(dot(normalize(pass_normal), normalize(-uni_light_direction)), 0, 1);
    vec3 diffuse_colour = (ambient * object_colour) + (object_colour * (uni_light_colour * light_factor));
	
	final_colour = vec4(diffuse_colour + specular_colour, 1.0);
	
}