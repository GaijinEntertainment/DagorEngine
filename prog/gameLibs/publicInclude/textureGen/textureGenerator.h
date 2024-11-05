//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DataBlock;
class TextureGenShader;
class TextureRegManager;
class TextureGenerator;
class TextureGenLogger;
class TextureGenEntitiesSaver;
class BaseTexture;

extern TextureGenerator *create_texture_generator();
void texgen_set_logger(TextureGenerator *, TextureGenLogger *logger);
TextureGenLogger *texgen_get_logger(TextureGenerator *);

extern TextureGenShader *texgen_get_shader(TextureGenerator *s, const char *name);

extern bool texgen_add_shader(TextureGenerator *s, const char *name, TextureGenShader *shader);
// returns number of commands executed or -1 on error
extern int texgen_start_process_commands(TextureGenerator &s, const DataBlock &commands, TextureRegManager &regs);
extern void texgen_stop_process_commands(TextureGenerator &s);
extern int texgen_process_commands(TextureGenerator &s, const unsigned count = 0xFFFFFF);
extern void delete_texture_generator(TextureGenerator *&gen);
extern const char *texgen_get_final_reg_name(TextureGenerator *s, const char *name);

extern bool add_pixel_shader_texgen(const DataBlock &shaders, TextureGenerator *texGen);

extern bool init_pixel_shader_texgen();
extern bool load_pixel_shader_texgen(const DataBlock &shaders);
extern void close_pixel_shader_texgen();

extern uint32_t extend_tex_fmt_to_32f(const BaseTexture *texture);
