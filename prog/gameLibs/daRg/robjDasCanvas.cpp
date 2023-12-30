#include "robjDasCanvas.h"
#include "guiScene.h"
#include "dasScripts.h"
#include <daRg/dasBinding.h>
#include "dargDebugUtils.h"

#include <daRg/dag_stringKeys.h>
#include <daRg/dag_uiShader.h>
#include <daRg/dag_element.h>
#include <daRg/dag_helpers.h>
#include <daRg/dag_renderState.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_picture.h>

#include <math/dag_mathUtils.h>
#include <gui/dag_stdGuiRender.h>


using namespace StdGuiRender;
using namespace das;

namespace darg
{

static bool verify_draw_func(DasScriptsData *das_script_data, FuncInfo *info, das::TypeInfo *data_arg_type)
{
  if (info->count != 3 && info->count != 4)
    return false;
  if (info->count == 4 && data_arg_type == nullptr)
    return false;

  TypeInfo *passTypes[3] = {
    das_script_data->typeGuiContextRef, das_script_data->typeConstElemRenderDataRef, das_script_data->typeConstRenderStateRef};

  for (uint32_t index = 0; index < 3; ++index)
  {
    auto argType = info->fields[index];
    if (!isValidArgumentType(argType, passTypes[index]))
      return false;
  }

  if (info->count == 4)
  {
    if (!isValidArgumentType(info->fields[3], data_arg_type))
      return false;
  }

  return true;
}


bool RobjDasCanvasParams::load(const Element *elem)
{
  GuiScene *scene = GuiScene::get_from_elem(elem);

  dasScriptObj.Release();
  drawFunc = nullptr;
  dasCtx = nullptr;
  dataArgType = nullptr;
  data.clear();

  if (!scene->dasScriptsData.get())
  {
    logerr("ROBJ_DAS_CANVAS requires daScript to be initialized");
    return false;
  }

  const Sqrat::Object &scriptDesc = elem->props.scriptDesc;
  Sqrat::Object drawFuncName = scriptDesc.GetSlot(elem->csk->drawFunc);
  if (drawFuncName.GetType() != OT_STRING)
  {
    darg_assert_trace_var("'drawFunc' parameter is missing", scriptDesc, elem->csk->rendObj);
    return false;
  }

  Sqrat::Object setupFuncName = scriptDesc.GetSlot(elem->csk->setupFunc);

  Sqrat::Object scriptObj = scriptDesc.GetSlot(elem->csk->script);
  if (scriptObj.GetType() != OT_INSTANCE || !Sqrat::ClassType<DasScript>::IsClassInstance(scriptObj.GetObject(), false))
  {
    darg_assert_trace_var("DAS canvas requires valid 'script' object", scriptDesc, elem->csk->rendObj);
  }
  else
  {
    DasScript *script = scriptObj.Cast<DasScript *>();
    das::Context *ctx = script->ctx.get();

    bool isUnique = false;

    if (setupFuncName.GetType() == OT_STRING)
    {
      das::SimFunction *setupFunc = ctx->findFunction(setupFuncName.GetVar<const char *>().value, isUnique);
      if (setupFunc)
      {
        das::FuncInfo *info = setupFunc->debugInfo;
        if (!isUnique)
          darg_assert_trace_var("'setup' function is not unique", scriptDesc, elem->csk->rendObj);
        else if (info->count != 2 || !isValidArgumentType(info->fields[0], scene->dasScriptsData->typeConstPropsRef))
          darg_assert_trace_var("'setup' function: invalid signature, expected arguments:"
                                "(props: Properties&; var storage: <user's struct> &)",
            scriptDesc, elem->csk->rendObj);
        else
        {
          das::TypeInfo *argType = info->fields[1];
          if (!argType->isPod() || argType->isConst() || !argType->isRefType() || argType->size <= 0)
            darg_assert_trace_var("'setup' function must accept non-const POD ref", scriptDesc, elem->csk->rendObj);
          else
          {
            dataArgType = argType;
            data.resize(argType->size);

            vec4f args[2] = {das::cast<const Properties &>::from(elem->props), das::cast<void *>::from(data.data())};
            ctx->eval(setupFunc, args, nullptr);

            ctx->restart();
            ctx->restartHeaps();
          }
        }
      }
    }


    das::SimFunction *draw = ctx->findFunction(drawFuncName.GetVar<const char *>().value, isUnique);
    if (!draw)
      darg_assert_trace_var("No 'draw' function found in das script", scriptDesc, elem->csk->rendObj);
    else if (!isUnique)
      darg_assert_trace_var("'draw' function is not unique", scriptDesc, elem->csk->rendObj);
    else if (!verify_draw_func(scene->dasScriptsData.get(), draw->debugInfo, dataArgType))
      darg_assert_trace_var(
        "Invalid signature of 'draw' function, expected aguments: "
        "(var ctx: GuiContext&; rdata: ElemRenderData& const; rstate: RenderState& const; [data: <user's struct> &])",
        scriptDesc, elem->csk->rendObj);
    else
    {
      this->dasScriptObj = scriptObj;
      this->dasCtx = ctx;
      this->drawFunc = draw;
    }
  }

  return true;
}


void RobjDasCanvas::renderCustom(GuiContext &ctx, const Element *elem, const ElemRenderData *rdata, const RenderState &render_state)
{
  if (rdata->size.x < 1 || rdata->size.y < 1)
    return;

  G_UNUSED(elem);
  G_UNUSED(render_state);
  RobjDasCanvasParams *params = static_cast<RobjDasCanvasParams *>(rdata->params);
  G_ASSERT_RETURN(params, );

  if (!params->drawFunc)
    return;

  GuiVertexTransform xf;
  ctx.getViewTm(xf.vtm);

  vec4f args[4] = {das::cast<GuiContext &>::from(ctx), das::cast<const ElemRenderData &>::from(*rdata),
    das::cast<const RenderState &>::from(render_state), das::cast<void *>::from(params->data.data())};
  params->dasCtx->eval(params->drawFunc, args, nullptr);

  params->dasCtx->restart();
  params->dasCtx->restartHeaps();

  ctx.setViewTm(xf.vtm);
}


} // namespace darg
