//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/vector.h>
#include <stdint.h>

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
// Starts a pass over commands; returns the number of nodes it runs.
extern int texgen_start_process_commands(TextureGenerator &s, const DataBlock &commands, TextureRegManager &regs);
// Ends the current pass; a node it stopped part-way cancels its remaining sub-steps, which can use the node's textures,
// so a host stops a pass before it releases that pass's registers.
extern void texgen_stop_process_commands(TextureGenerator &s);
extern int texgen_process_commands(TextureGenerator &s, const unsigned count = 0xFFFFFF);
// GPU sync after each pixel-shader command (one sub-step of a node), for every TextureGenerator in the process.
// FlushAndWait (default) flushes and waits for the GPU; None leaves it queued for the caller to bound. Set it while no pass runs.
enum class TextureGenCommandSync
{
  FlushAndWait,
  None
};
extern void texgen_set_command_sync(TextureGenCommandSync mode);
// Tile side cap in pixels for the sub-steps of pixel-shader nodes without particles, for every TextureGenerator in the process;
// 0 (default) or less keeps each shader's own size, and a positive cap below 64 acts as 64. Set it while no pass runs.
extern void texgen_set_max_tile_size(int size);
extern int texgen_get_max_tile_size();
// The command the next texgen_process_commands(s, 1) runs.
struct TextureGenCommand
{
  const char *shaderName = nullptr;
  // Equal for commands that report the same shader name in nodes whose blocks match apart from the node name and register names.
  // The host predicts such commands at the same GPU time per pixel.
  uint64_t costKey = 0;
  // The area the command covers, or 0 while its node has not started (the node size is not known yet).
  int pixels = 0;
  // The command can block until all queued GPU work is done, so the host ends its batch before it.
  bool mapsOnCpu = false;
  // The command starts a node with an input file that TextureRegManager::isFileLoadPending reports.
  bool loadsFiles = false;
  // The command starts a node that may create textures: a changed live output, a new output with no pooled match, a second new output,
  // or a first-use input that TextureRegManager::isGeneratedTexturePending reports. Scratch targets of process are not seen.
  bool createsTextures = false;
};
// False when the pass has no command left.
extern bool texgen_get_next_command(TextureGenerator &s, TextureGenCommand &command);
// Input registers named "tex:..." of the next node_count nodes to start; the names stay valid until the next pass start.
extern void texgen_get_upcoming_tex_inputs(TextureGenerator &s, int node_count, eastl::vector<const char *> &reg_names);
// Area the command the last texgen_process_commands(s, 1) ran covered; a call that runs no command leaves it unchanged.
extern int texgen_get_last_command_pixels(TextureGenerator &s);
// Nodes the current pass has finished, out of the count texgen_start_process_commands returned.
extern int texgen_get_done_node_count(TextureGenerator &s);
// Block name of the node the pass runs now; nullptr outside a pass.
extern const char *texgen_get_current_node_name(TextureGenerator &s);
// Block names of the nodes the current pass runs; the names stay valid until the next start.
extern void texgen_get_pass_node_names(TextureGenerator &s, eastl::vector<const char *> &names);
// Content hash of the named node as of the last start, which decides whether a pass reruns it; 0 when that start had no
// such node, or when a stopped pass did not reach it.
extern uint64_t texgen_get_node_key(TextureGenerator &s, const char *name);
extern void delete_texture_generator(TextureGenerator *&gen);
// Pins the record each cache_node of commands reads in place of its input; call it before each pass and regcache::update.
extern void texgen_pin_cache_node_records(const DataBlock &commands);
// Block names of the nodes that a final of commands reads, directly or through other nodes, the inputs of each cache_node
// included. The names stay valid while commands does not change.
extern void texgen_get_used_node_names(const DataBlock &commands, eastl::vector<const char *> &names);
// The key of the regcache record that a cache_node block writes, and reads in place of its input later; nullptr for another node.
// It stays valid while node does not change.
extern const char *texgen_get_cache_node_record_id(const DataBlock &node);
// Block names of the nodes that the named node of commands reads, directly or through other nodes, never the node itself;
// none if no such node. The names stay valid while commands does not change.
extern void texgen_get_node_input_names(const DataBlock &commands, const char *node, eastl::vector<const char *> &names);
extern const char *texgen_get_final_reg_name(TextureGenerator *s, const char *name);
// cb gets each final as soon as the node that writes it ends; the final then holds no use of its register, which is
// released as any other register is. With no callback a final stays after the pass. Set it while no pass runs; it
// applies from the next texgen_start_process_commands.
typedef void (*TextureGenFinalCallback)(void *ctx, const char *final_name, BaseTexture *tex);
extern void texgen_set_final_callback(TextureGenerator *s, TextureGenFinalCallback cb, void *ctx);
// Tries of the node order search per pass, 2000 by default, in rounds of 2000 that each start from the best order so far,
// so more tries never raise the peak of the search's memory model. A pass with the same nodes, inputs, sizes, finals,
// scratch pad registers and selected block as the one before goes on from the order that pass found, also with 0 tries;
// else 0 tries keep the Appel order.
extern void texgen_set_order_search_evals(TextureGenerator *s, int evals);

extern bool add_pixel_shader_texgen(const DataBlock &shaders, TextureGenerator *texGen);

extern bool init_pixel_shader_texgen();
extern bool load_pixel_shader_texgen(const DataBlock &shaders);
extern void close_pixel_shader_texgen();

extern uint32_t extend_tex_fmt_to_32f(const BaseTexture *texture);

// Hashes the params (name, type, value bits) and sub-blocks of blk; the result does not depend on the BLK name map.
extern uint64_t texgen_hash_block_content(const DataBlock &blk);
