#include <main_state.h>
#include <glad/glad.h>
#include <math.h>


#include <rafgl.h>

#include <game_constants.h>


rafgl_meshPUN_t telo, kupola, cev, gusenica_l, gusenica_r;

rafgl_meshPUN_t plane;

rafgl_texture_t heightmap, tank_tex, track_tex, grass;


rafgl_raster_t raster_hm;

void load_tex(rafgl_texture_t *tex, const char *path)
{
    rafgl_raster_t raster;
    rafgl_raster_load_from_image(&raster, path);
    rafgl_texture_init(tex);
    rafgl_texture_load_from_raster(tex, &raster);

    glBindTexture(GL_TEXTURE_2D, tex->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

}


typedef struct
{
    GLuint program_id;
    GLuint uni_M, uni_V, uni_P;

    GLuint uni_light_colour;
    GLuint uni_light_direction;
    GLuint uni_camera_position;


    GLuint special_unis[16];

    void (*tex_fun)(GLuint);

} shader_program_t;

shader_program_t tank_shader;
shader_program_t terrain_shader;

void tank_tex_fun(GLuint prog_id)
{
    glUniform1i(glGetUniformLocation(prog_id, "albedo"), 0);
}

void terrain_tex_fun(GLuint prog_id)
{
    glUniform1i(glGetUniformLocation(prog_id, "heightmap"), 0);
    glUniform1i(glGetUniformLocation(prog_id, "albedo"), 1);
}

void load_tank_shader()
{
    GLuint id = tank_shader.program_id = rafgl_program_create_from_name("lambert_shader");
    tank_shader.tex_fun = tank_tex_fun;
    tank_shader.uni_M = glGetUniformLocation(id, "uni_M");
    tank_shader.uni_V = glGetUniformLocation(id, "uni_V");
    tank_shader.uni_P = glGetUniformLocation(id, "uni_P");

    tank_shader.uni_light_colour = glGetUniformLocation(id, "uni_light_colour");
    tank_shader.uni_light_direction = glGetUniformLocation(id, "uni_light_direction");
    tank_shader.uni_camera_position = glGetUniformLocation(id, "uni_camera_position");


}

void load_terrain_shader()
{
    GLuint id = terrain_shader.program_id = rafgl_program_create_from_name("terrain_shader");
    terrain_shader.tex_fun = terrain_tex_fun;
    terrain_shader.uni_M = glGetUniformLocation(id, "uni_M");
    terrain_shader.uni_V = glGetUniformLocation(id, "uni_V");
    terrain_shader.uni_P = glGetUniformLocation(id, "uni_P");

    terrain_shader.uni_light_colour = glGetUniformLocation(id, "uni_light_colour");
    terrain_shader.uni_light_direction = glGetUniformLocation(id, "uni_light_direction");
    terrain_shader.uni_camera_position = glGetUniformLocation(id, "uni_camera_position");

}

void bind_shader(shader_program_t *sp)
{
    if(sp != NULL)
    {
        glUseProgram(sp->program_id);
        sp->tex_fun(sp->program_id);
    }
    else
    {
        glUseProgram(0);
    }
}


vec3_t light_direction;
vec3_t light_colour;


void main_state_init(GLFWwindow *window, void *args, int width, int height)
{
    light_direction = v3_norm(vec3(-1, -1, -1));
    light_colour = vec3(1.0f, 0.9f, 0.7f);


    rafgl_meshPUN_init(&plane);
    rafgl_meshPUN_load_plane(&plane, 40, 40, 64, 64);


    rafgl_meshPUN_init(&telo);
    rafgl_meshPUN_init(&kupola);
    rafgl_meshPUN_init(&cev);
    rafgl_meshPUN_init(&gusenica_l);
    rafgl_meshPUN_init(&gusenica_r);

    rafgl_meshPUN_load_from_OBJ(&telo, "res/models/tank-base.obj");
    rafgl_meshPUN_load_from_OBJ(&cev, "res/models/tank-cijev.obj");
    rafgl_meshPUN_load_from_OBJ(&kupola, "res/models/tank-kupola.obj");
    rafgl_meshPUN_load_from_OBJ(&gusenica_l, "res/models/tank-track-L.obj");
    rafgl_meshPUN_load_from_OBJ(&gusenica_r, "res/models/tank-track-R.obj");

    load_tank_shader();
    load_terrain_shader();


    load_tex(&tank_tex, "res/images/tank-tex.png");
    load_tex(&track_tex, "res/images/tracks.png");
    load_tex(&heightmap, "res/images/noise.png");
    load_tex(&grass, "res/images/grass.jpg");

    rafgl_raster_load_from_image(&raster_hm, "res/images/noise.png");


}

mat4_t model, view, projection, view_projection;
mat4_t modelBase, modelKupola, modelCev, modelGL, modelGD;

/* field of view */
float fov = 75.0f;

vec3_t camera_position = vec3m(0.0f, 1.0f, 6.5f);
vec3_t camera_up = vec3m(0.0f, 1.0f, 0.0f);
vec3_t aim_dir = vec3m(0.0f, 0.0f, -1.0f);

float camera_angle = -M_PIf * 0.5f;
float angle_speed = 0.2f * M_PIf;
float move_speed = 2.4f;

float hoffset = 0;

int rotate = 0;
float model_angle = 0.0f;

void v3show(vec3_t v)
{
    printf("(%.2f %.2f %.2f)\n", v.x, v.y, v.z);
}

float time = 0.0f;
int reshow_cursor_flag = 0;
int last_lmb = 0;

float sensitivity = 1.0f;

float ugao_kupole = 0.0f;;

float recoil = 0.0;

vec3_t tank_position = vec3m(0, 0, 0);
float ugao_tenka = 0.0f;

float LGOffset = 0.0f;
float DGOffset = 0.0f;

float LGSpeed = 0.0f;
float DGSpeed = 0.0f;

void main_state_update(GLFWwindow *window, float delta_time, rafgl_game_data_t *game_data, void *args)
{


    time += delta_time;
    model_angle += delta_time * rotate;


    if(!game_data->keys_down[RAFGL_KEY_LEFT_SHIFT])
    {
        angle_speed = 0.2f * M_PIf;
        move_speed = 2.4f;
        sensitivity = 1.0f;
    }
    else
    {
        angle_speed = 5 * 0.2f * M_PIf;
        move_speed = 5 * 2.4f;
        sensitivity = 1.0f;
    }


    if(game_data->is_lmb_down)
    {

        if(reshow_cursor_flag == 0)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }

        float ydelta = game_data->mouse_pos_y - game_data->raster_height / 2;
        float xdelta = game_data->mouse_pos_x - game_data->raster_width / 2;

        if(!last_lmb)
        {
            ydelta = 0;
            xdelta = 0;
        }

        hoffset -= sensitivity * ydelta / game_data->raster_height;
        camera_angle += sensitivity * xdelta / game_data->raster_width;

        glfwSetCursorPos(window, game_data->raster_width / 2, game_data->raster_height / 2);
        reshow_cursor_flag = 1;
    }
    else if(reshow_cursor_flag)
    {
        reshow_cursor_flag = 0;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    last_lmb = game_data->is_lmb_down;


    LGSpeed = 0;
    DGSpeed = 0;

    if(game_data->keys_pressed['R']) rotate = !rotate;

    if(game_data->keys_down['D'])
    {
        ugao_tenka -= delta_time;
        LGSpeed += 1;
        DGSpeed -= 1;
    }

    if(game_data->keys_down['A'])
    {
        ugao_tenka += delta_time;
        LGSpeed += -1;
        DGSpeed -= -1;
    }

    aim_dir = v3_norm(vec3(sinf(ugao_tenka), 0, cosf(ugao_tenka)));

    if(game_data->keys_down['S'])
    {
        tank_position = v3_add(tank_position, v3_muls(aim_dir, move_speed * delta_time));
        LGSpeed += -1;
        DGSpeed += -1;
    }

    if(game_data->keys_down['W'])
    {
        tank_position = v3_add(tank_position, v3_muls(aim_dir, -move_speed * delta_time));
        LGSpeed += 1;
        DGSpeed += 1;
    }

    LGOffset += LGSpeed * delta_time;
    DGOffset += DGSpeed * delta_time;

    if(game_data->keys_down['E']) ugao_kupole -= delta_time;
    if(game_data->keys_down['Q']) ugao_kupole += delta_time;

    if(game_data->keys_pressed['F'])
    {
        recoil += 0.5f;
        //if(recoil > 2.0f) recoil = 2.0f;
    }

    if(game_data->keys_down[RAFGL_KEY_ESCAPE]) glfwSetWindowShouldClose(window, GLFW_TRUE);

    if(game_data->keys_down[RAFGL_KEY_SPACE]) camera_position.y += 1.0f * delta_time * move_speed;
    if(game_data->keys_down[RAFGL_KEY_LEFT_CONTROL]) camera_position.y -= 1.0f * delta_time * move_speed;

    float aspect = ((float)(game_data->raster_width)) / game_data->raster_height;

    projection = m4_perspective(90.0f, aspect, 0.1f, 100.0f);


    vec3_t map_uv = v3_adds(tank_position, 20);
    map_uv = v3_muls(map_uv, 1.0f / 40.0f);

    float height = rafgl_bilinear_sample(&raster_hm, map_uv.x, map_uv.z).r * 2.0f / 256.0f;

    vec3_t map_uv2 = v3_add(tank_position, v3_muls(aim_dir, -move_speed * 0.02));


    float distance = v3_length(v3_muls(aim_dir, -move_speed * 0.02));

    float height2 = rafgl_bilinear_sample(&raster_hm, map_uv2.x, map_uv2.z).r * 2.0f / 256.0f;

    float pitch = atan2(height - height2, distance);

    tank_position.y = height;

    camera_position = v3_add(tank_position, vec3m(0.0f, 3.5, 4));


    view = m4_look_at(camera_position, tank_position, camera_up);

    modelBase = m4_mul(m4_mul(m4_translation(tank_position) ,  m4_rotation_y(ugao_tenka)), m4_rotation_x(pitch));

    modelKupola = m4_mul(modelBase , m4_rotation_y(ugao_kupole));



    modelCev = m4_mul(modelKupola , m4_translation(vec3(0, 0, recoil)));

    vec3_t vektor_ciljanja = m4_mul_dir(modelCev, vec3(0, 0, 1));

    recoil -= delta_time;
    if(recoil < 0) recoil = 0;


    modelGD = modelBase;
    modelGL = modelBase;

    tank_position.y = 0;

}


void load_vec3(GLuint location, vec3_t v)
{
    glUniform3f(location, v.x, v.y, v.z);
}

void load_mat4(GLuint location, mat4_t *m)
{
    glUniformMatrix4fv(location, 1, GL_FALSE, &(m->m00));
}

void main_state_render(GLFWwindow *window, void *args)
{

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    bind_shader(&terrain_shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightmap.tex_id);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, grass.tex_id);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindVertexArray(plane.vao_id);

    load_vec3(terrain_shader.uni_light_colour, light_colour);
    load_vec3(terrain_shader.uni_light_direction, light_direction);
    load_vec3(terrain_shader.uni_camera_position, camera_position);

    mat4_t identity = m4_identity();
    load_mat4(terrain_shader.uni_M, &identity);
    load_mat4(terrain_shader.uni_V, &view);
    load_mat4(terrain_shader.uni_P, &projection);


    glDrawArrays(GL_TRIANGLES, 0, plane.vertex_count);


    bind_shader(&tank_shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tank_tex.tex_id);

    glBindVertexArray(telo.vao_id);

    load_vec3(tank_shader.uni_light_colour, light_colour);
    load_vec3(tank_shader.uni_light_direction, light_direction);
    load_vec3(tank_shader.uni_camera_position, camera_position);

    glUniform1f(glGetUniformLocation(tank_shader.program_id, "v_scale"), 1.0f);
    glUniform1f(glGetUniformLocation(tank_shader.program_id, "v_offset"), 0.0f);

    load_mat4(tank_shader.uni_V, &view);
    load_mat4(tank_shader.uni_P, &projection);

    load_mat4(tank_shader.uni_M, &modelBase);
    glDrawArrays(GL_TRIANGLES, 0, telo.vertex_count);


    glBindVertexArray(kupola.vao_id);
    load_mat4(tank_shader.uni_M, &modelKupola);

    glDrawArrays(GL_TRIANGLES, 0, kupola.vertex_count);

    glBindVertexArray(cev.vao_id);
    load_mat4(tank_shader.uni_M, &modelCev);

    glDrawArrays(GL_TRIANGLES, 0, cev.vertex_count);

    glBindVertexArray(gusenica_l.vao_id);
    load_mat4(tank_shader.uni_M, &modelGL);



    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, track_tex.tex_id);

    glUniform1f(glGetUniformLocation(tank_shader.program_id, "v_scale"), 1.0f);
    glUniform1f(glGetUniformLocation(tank_shader.program_id, "v_offset"), LGOffset);
    glDrawArrays(GL_TRIANGLES, 0, gusenica_l.vertex_count);

    glBindVertexArray(gusenica_r.vao_id);
    load_mat4(tank_shader.uni_M, &modelGD);

    glUniform1f(glGetUniformLocation(tank_shader.program_id, "v_scale"), 1.0f);
    glUniform1f(glGetUniformLocation(tank_shader.program_id, "v_offset"), DGOffset);
    glDrawArrays(GL_TRIANGLES, 0, gusenica_r.vertex_count);



    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
}


void main_state_cleanup(GLFWwindow *window, void *args)
{

}
