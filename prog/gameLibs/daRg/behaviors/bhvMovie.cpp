#include "bhvMovie.h"
#include "guiScene.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_scriptHandlers.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
#include <videoPlayer/dag_videoPlayer.h>
#include <daRg/dag_uiSound.h>
#include <sound/dag_genAudio.h>

namespace darg
{


BhvMovie bhv_movie;


BhvMovie::BhvMovie() : Behavior(STAGE_BEFORE_RENDER, 0) {}


static IGenSound *create_sound_for_videofile(const char *fn)
{
  if (!fn || !*fn)
    return nullptr;

  // sound file should have name <video_file.ext>.mp3
  String snd_file(0, "%s.mp3", fn);
  if (dd_file_exists(snd_file.c_str()))
  {
    return ui_sound_player->getAudioSystem()->createSound(snd_file.c_str());
  }
  return nullptr;
}

static IGenVideoPlayer *create_player_for_file(const char *fn)
{
  if (!fn || !*fn)
    return nullptr;

  if (!dd_file_exists(fn))
  {
    logwarn("Video file '%s' not found", fn);
    return nullptr;
  }

  const int buf_num = 4;

  const char *fext = dd_get_fname_ext(fn);
  if (fext && dd_stricmp(fext, ".ivf") == 0)
    return IGenVideoPlayer::create_av1_video_player(fn, buf_num);
  else
    return IGenVideoPlayer::create_ogg_video_player(fn, buf_num);
}


static void init_player(darg::Element *elem)
{
  Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  Sqrat::Var<const char *> fn = scriptDesc.RawGetSlot(elem->csk->movie).GetVar<const char *>();

  IGenVideoPlayer *player = create_player_for_file(fn.value);
  IGenSound *soundPlayer = nullptr;
  if (!player)
  {
    Sqrat::Object handler = elem->props.scriptDesc.RawGetSlot("onError");
    if (!handler.IsNull())
    {
      Sqrat::Function f(handler.GetVM(), elem->props.scriptDesc, handler);
      GuiScene *scene = GuiScene::get_from_elem(elem);
      scene->queueScriptHandler(new ScriptHandlerSqFunc<>(f));
    }
    return;
  }

  elem->props.storage.SetInstance(elem->csk->moviePlayer, player);
  elem->props.storage.SetValue(elem->csk->movieFileName, fn.value);

  bool loop = scriptDesc.RawGetSlotValue(elem->csk->loop, true);

  if (!loop)
    soundPlayer = create_sound_for_videofile(fn.value);

  if (soundPlayer)
    elem->props.storage.SetInstance(elem->csk->soundPlayer, soundPlayer);

  IGenVideoPlayer *playerCheck = elem->props.storage.RawGetSlotValue<IGenVideoPlayer *>(elem->csk->moviePlayer, nullptr);
  G_UNUSED(playerCheck);
  G_ASSERTF(playerCheck == player, "Players mismatch %p | %p", player, playerCheck);

  player->setAutoRewind(loop);
  player->start();
  if (soundPlayer)
    ui_sound_player->getAudioSystem()->playSound(soundPlayer);
}


static void close_player(darg::Element *elem)
{
  IGenVideoPlayer *player = elem->props.storage.RawGetSlotValue<IGenVideoPlayer *>(elem->csk->moviePlayer, nullptr);
  if (player)
  {
    player->stop(true);
    player->destroy();
    elem->props.storage.DeleteSlot(elem->csk->moviePlayer);
  }

  IGenSound *soundPlayer = elem->props.storage.RawGetSlotValue<IGenSound *>(elem->csk->soundPlayer, nullptr);
  if (soundPlayer)
  {
    darg::ui_sound_player->getAudioSystem()->releaseSound(soundPlayer);
    elem->props.storage.DeleteSlot(elem->csk->soundPlayer);
  }

  elem->props.storage.DeleteSlot(elem->csk->movieFileName);
  elem->props.storage.DeleteSlot(elem->csk->isPlaying);
}


void BhvMovie::onElemSetup(Element *elem, SetupMode setup_mode)
{
  if (setup_mode == SM_REBUILD_UPDATE)
  {
    Sqrat::Var<const char *> newFn = elem->props.scriptDesc.RawGetSlot(elem->csk->movie).GetVar<const char *>();
    Sqrat::Var<const char *> curFn = elem->props.storage.RawGetSlot(elem->csk->movieFileName).GetVar<const char *>();
    if ((!newFn.value != !curFn.value) || (newFn.value && curFn.value && strcmp(newFn.value, curFn.value) != 0))
    {
      close_player(elem);
      init_player(elem);
    }
  }
}


void BhvMovie::onAttach(darg::Element *elem) { init_player(elem); }


void BhvMovie::onDetach(darg::Element *elem, DetachMode) { close_player(elem); }


int BhvMovie::update(UpdateStage, Element *elem, float /*dt*/)
{
  IGenVideoPlayer *player = elem->props.storage.RawGetSlotValue<IGenVideoPlayer *>(elem->csk->moviePlayer, nullptr);
  if (!player)
    return 0;

  bool wasPlaying = elem->props.storage.RawGetSlotValue(elem->csk->isPlaying, false);
  bool isPlaying = !player->isFinished();

  if (isPlaying)
  {
    // timeQuantUsec = obj->finalProps.getPropInt(timeQuantUsecId, 8000);
    int timeQuantUsec = 8000;

    player->advance(timeQuantUsec);
  }

  if (isPlaying != wasPlaying)
  {
    Sqrat::Object handler = elem->props.scriptDesc.RawGetSlot(isPlaying ? "onStart" : "onFinish");
    if (!handler.IsNull())
    {
      Sqrat::Function f(handler.GetVM(), elem->props.scriptDesc, handler);
      GuiScene *scene = GuiScene::get_from_elem(elem);
      scene->queueScriptHandler(new ScriptHandlerSqFunc<>(f));
    }
  }

  elem->props.storage.SetValue(elem->csk->isPlaying, isPlaying);

  return 0;
}


} // namespace darg
