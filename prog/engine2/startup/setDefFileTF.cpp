#include <startup/dag_startupTex.h>
#include <3d/dag_texMgr.h>
#include <3d/fileTexFactory.h>

void set_default_file_texture_factory() { set_default_tex_factory(&FileTextureFactory::self); }

void set_missing_texture_name(const char *tex_fname, bool replace_bad_shader_textures)
{
  FileTextureFactory::self.setMissingTexture(tex_fname, replace_bad_shader_textures);
}

void set_missing_texture_usage(bool replace_missing_tex) { FileTextureFactory::self.setMissingTextureUsage(replace_missing_tex); }
