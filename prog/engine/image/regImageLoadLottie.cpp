#include <3d/dag_drv3dCmd.h>
#include <ioSys/dag_dataBlock.h>
#include <gui/dag_stdGuiRender.h>
#include <osApiWrappers/dag_miscApi.h>
#include <3d/dag_picMgr.h>
#include <debug/dag_debug3d.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_atomic.h>
#include <math/integer/dag_IPoint2.h>
#include <perfMon/dag_cpuFreq.h>

#include <perfMon/dag_statDrv.h>
#include <generic/dag_initOnDemand.h>
#include <3d/dag_dynAtlas.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/vector_map.h>
#include <EASTL/vector.h>
#include <EASTL/queue.h>
#include <EASTL/set.h>
#include <EASTL/string.h>
#include <util/dag_simpleString.h>
#include <util/dag_hash.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_threads.h>

#define LOTTIE_STATIC 1
#include <lottie/inc/rlottie.h>

#define DEBUG_LOTTIE_UPDATE 0

struct CopyRecTx
{
  // rect in sc texture
  DynamicAtlasFencedTexUpdater::CopyRec cr;
  // rect in PM where placed image (float)
  Point2 out_lt, out_rb;
  // texture id in PM where placed image
  TEXTUREID tId;
};

#if DEBUG_LOTTIE_UPDATE
struct LottieUpdateDebug
{
  const char *lottie;
  uint32_t frame;
  uint32_t duration;
};
#endif

struct LockedTexInfo
{
#if DEBUG_LOTTIE_UPDATE
  eastl::vector<LottieUpdateDebug> copy_rects_d;
#endif
  // rects which need copy from sysTex to dst text
  eastl::vector<CopyRecTx> copy_rects;
};

// mininmal info about lottie aninmation, need
// for fast check we pid assigned for any animation
struct LottieAnimDesc
{
  IPoint2 size;
  PICTUREID pid = BAD_PICTUREID;
};

// Data in system memory where saved frame
struct LottieReadyFrame
{
  Tab<char> data;
  IPoint2 size;
  union
  {
    PICTUREID pid = BAD_PICTUREID;
    uint32_t frame;
  };
#if DEBUG_LOTTIE_UPDATE
  const char *lottie = nullptr;
  uint32_t frame_dbg = 0;
  uint32_t duration_ms = 0;
#endif
};

struct LottieAnim
{
  // default size for lottie renderer
  static constexpr int DEFAULT_SIZE = 32;
  // how many prerendered frames saved in array
  static constexpr int DEFAULT_PRERENDERED_FRAMES = 3; // To consider: lower this number for lower end systems?
  static constexpr int LOTTIE_SURFACE_FMT = TEXFMT_A8R8G8B8;
  static constexpr int LOTTIE_SURFACE_FMT_BPP = sizeof(uint32_t);

  PICTUREID pid = BAD_PICTUREID;
  struct
  {
    int width = DEFAULT_SIZE;
    int height = DEFAULT_SIZE;
  } canvas;
  uint32_t bufSize() const { return canvas.width * canvas.height * LOTTIE_SURFACE_FMT_BPP; }
  struct
  {
    uint32_t duration_ms = 0;
    uint32_t start_ms = 0;
  } timeline;
  struct
  {
    uint16_t total = 0;
  } frame;
  bool loop = false;
  bool play = false;
  uint32_t maxPrerenderedFrames = DEFAULT_PRERENDERED_FRAMES;

  eastl::unique_ptr<rlottie::Animation> anim;
  // we need save future frames, because are can have
  // different time for render, thread render it on loop
  eastl::queue<LottieReadyFrame> prerenderedFrames;

  // here saved frame, which need for display, on every render()
  // call prerendered frame will moved here when time for next frame gone
  LottieReadyFrame currentFrame;

#if DEBUG_LOTTIE_UPDATE || (DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN)
  String lottiePath;
#endif

  static uint32_t getPropsHash(const DataBlock &pic_props)
  {
    String hash(0, "lottie:%s|canvasHeight:%d|canvasWidth:%d|loop:%d|rate:%d", pic_props.getStr("lottie"),
      pic_props.getInt("canvasWidth", DEFAULT_SIZE), pic_props.getInt("canvasHeight", DEFAULT_SIZE),
      pic_props.getBool("loop", false) ? 1 : 0, pic_props.getInt("rate", -1));
    return str_hash_fnv1(hash.c_str());
  }

  static IPoint2 getSizeFromBlk(const DataBlock &pic_props)
  {
    int w = eastl::max(1, pic_props.getInt("canvasWidth", DEFAULT_SIZE));
    int h = eastl::max(1, pic_props.getInt("canvasHeight", DEFAULT_SIZE));

    int scale = 1;
    if (w < DEFAULT_SIZE || h < DEFAULT_SIZE)
      scale = 2;

    return {w * scale, h * scale};
  }

  bool load(const DataBlock &pic_props)
  {
    const char *path = pic_props.getStr("lottie");
    if (!path || 0 == *path)
      return false;

    IPoint2 size = getSizeFromBlk(pic_props);
    canvas.width = size.x;
    canvas.height = size.y;

    loop = pic_props.getBool("loop", false);
    play = pic_props.getBool("play", false);
    maxPrerenderedFrames = eastl::max(pic_props.getInt("prerenderedFrames", DEFAULT_PRERENDERED_FRAMES), 1);

    if (file_ptr_t fp = df_open(path, DF_READ))
    {
#if DEBUG_LOTTIE_UPDATE || (DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN)
      lottiePath = path;
#endif
      Tab<char> data;
      data.resize(df_length(fp));
      df_read(fp, data.data(), (int)data.size());
      df_close(fp);

      const char *cacheKey = "";
      const eastl::string resourcePath = ""; // we not use external resources
      anim.reset(rlottie::Animation::loadFromData(eastl::string(data.data(), data.size()).c_str(), eastl::string(cacheKey).c_str(),
        resourcePath.c_str(), false /*not use cache*/)
                   .release());
    }

    if (anim)
    {
      int customRate = pic_props.getInt("rate", -1);
      frame.total = (uint16_t)eastl::min((int)anim->totalFrame(), USHRT_MAX);
      float oneFrameMs = anim->duration() / frame.total;
      timeline.duration_ms = customRate > 0 ? 1000.f / customRate : oneFrameMs * 1000;
      debug("<%s>: loaded lottie animation (frames:%d frame_dur_ms:%d WxH:%dx%d)", path, frame.total, timeline.duration_ms,
        canvas.width, canvas.height);
    }
    else
    {
      logerr("Lottie::animation load failed from <%s>", path);
      return false;
    }

    return true;
  }

  bool render(uint32_t curTime)
  {
    if (pid == BAD_PICTUREID || !play)
      return false;

    uint32_t curFrame = (curTime - timeline.start_ms) / timeline.duration_ms;

    if (prerenderedFrames.empty())
      while (prerenderedFrames.size() < maxPrerenderedFrames)
      {
        // calc next prerendered frame index
        uint32_t nextFrameIndex = curFrame + prerenderedFrames.size();
        if (!loop && nextFrameIndex >= frame.total)
          break;

        // create new frame
        prerenderedFrames.push({});
        LottieReadyFrame &nextFrame = prerenderedFrames.back();
        nextFrame.frame = nextFrameIndex;

        // create memory block where will be placed frame
        nextFrame.data.resize(bufSize());

        // save frame size for next actions
        nextFrame.size = {canvas.width, canvas.height};

        // create rlottie structure for rasterize animation frame
        rlottie::Surface surface((uint32_t *)nextFrame.data.data(), canvas.width, canvas.height,
          canvas.width * LOTTIE_SURFACE_FMT_BPP);

        // rasterize frame to nextFrame.data, rlottie::Surface is temporary structure which not save any data
        anim->renderSync(nextFrameIndex % frame.total, surface);
      }

    // move first of prerendered frames to readyFrame, main thread
    // after render it will be move to readyFrames array
    if (!prerenderedFrames.empty() && curFrame >= prerenderedFrames.front().frame)
    {
      currentFrame = eastl::move(prerenderedFrames.front());
      prerenderedFrames.pop();
      currentFrame.pid = pid;
#if DEBUG_LOTTIE_UPDATE
      currentFrame.lottie = lottiePath.c_str();
      currentFrame.frame_dbg = curFrame % frame.total;
      currentFrame.duration_ms = timeline.duration_ms;
#endif

      return true;
    }

    return false;
  }
};

struct LottieRenderCommand
{
  enum Type
  {
    UNKNOWN = 0,
    ADD_CONFIG,
    DISCARD_PID,
    SETUP_PID,
    SETUP_PLAY
  };
  Type type;
  DataBlock config;
  PICTUREID pid;
  bool play;
};

// this thread resolve command to load lotti animations, and their render frames
struct LottieRenderThread final : public DaThread
{
  LottieRenderThread() : DaThread("LottieRenderThread") {}

  bool execOneCommand()
  {
    WinAutoLock lock(commandsMutex);
    if (commands.empty())
      return false;

    LottieRenderCommand cmd(eastl::move(commands.front()));
    commands.pop();
    bool empty = commands.empty();

    lock.unlockFinal();

    execCommand(cmd);

    return !empty;
  }

  // addCommand() only push it to queue without execute
  template <typename... Args>
  void addCommand(const Args &...args)
  {
    G_ASSERT(DaThread::isThreadStarted()); // Note: match() starts it (expected to be called at this point)
    WinAutoLock lock(commandsMutex);
    commands.push({args...});
  }

  eastl::vector_map<uint32_t, LottieAnim> animations;
  uint32_t minDurationMs = 100;

  // this queue contain commands for animations
  // load - load animation may take much time
  // discard - after reset\remove image in PM we need remove it from quese
  //           because PM cant contain real image for it
  // setup pid - animation need assign PictureID in PM, this pid
  //             received later than load, when PM have area in atlas for image
  // setup play flag - for future, when we need change play status
  WinCritSec commandsMutex;
  eastl::queue<LottieRenderCommand> commands;

  // this queue contain ready frames from animations, it placed in system
  // memory that another thread can copy their to PM texture later
  WinCritSec readyFramesMutex;
  eastl::queue<LottieReadyFrame> readyFrames;

  void pushReadyFrame(LottieReadyFrame &frame, size_t maxAnimSize)
  {
    WinAutoLock lock(readyFramesMutex);
    // remove extra frames, that avoid creating infinite queue
    if (readyFrames.size() > maxAnimSize)
      readyFrames.pop();

    readyFrames.push(eastl::move(frame));
  }

  bool popReadyFrame(LottieReadyFrame &frame)
  {
    WinAutoLock lock(readyFramesMutex);
    if (readyFrames.empty())
      return false;

    // FIXME: this will free 'frame.data' buffer later. Use proper double buffering to avoid doing that
    eastl::swap(frame, readyFrames.front());
    readyFrames.pop();

    return true;
  }

  // resolve command in thread, because it can be added async from another thread
  void execCommand(const LottieRenderCommand &cmd)
  {
    switch (cmd.type)
    {
      case LottieRenderCommand::ADD_CONFIG:
      {
        LottieAnim anim;
        const uint32_t propsHash = LottieAnim::getPropsHash(cmd.config);

        bool loadOk = anim.load(cmd.config);
        if (loadOk)
        {
          minDurationMs = eastl::min(anim.timeline.duration_ms, minDurationMs);
          animations.insert(eastl::make_pair(+propsHash, eastl::move(anim)));
        }
      }
      break;

      case LottieRenderCommand::DISCARD_PID:
      {
        auto it = eastl::find_if(animations.begin(), animations.end(), [pid = cmd.pid](auto &a) { return a.second.pid == pid; });
        if (it != animations.end())
          animations.erase(it);
      }
      break;

      case LottieRenderCommand::SETUP_PID:
      {
        const uint32_t propsHash = LottieAnim::getPropsHash(cmd.config);
        auto it = animations.find(propsHash);
        if (it != animations.end())
        {
          it->second.pid = cmd.pid;
          it->second.timeline.start_ms = get_time_msec();
        }
      }
      break;

      case LottieRenderCommand::SETUP_PLAY:
      {
        auto it = eastl::find_if(animations.begin(), animations.end(), [pid = cmd.pid](auto &a) { return a.second.pid == pid; });
        if (it != animations.end())
          it->second.play = cmd.play;
      }
      break;

      default: break;
    }
  }

  void execute() override
  {
    while (!interlocked_acquire_load(terminating))
    {
      while (execOneCommand())
        ;

      if (animations.empty())
      {
        sleep_msec(1000);
        continue;
      }

      // render animations and extract current animation frame to ready frames array
      uint32_t curtime = get_time_msec();
      const size_t maxAnimSize = animations.size() * 2;
      for (auto &anim : animations)
      {
        // it's loop here for all animations and frame render make a time, break it when thread want stop
        if (interlocked_acquire_load(terminating))
          return;

        if (anim.second.render(curtime))
          pushReadyFrame(anim.second.currentFrame, maxAnimSize);
      }

      sleep_msec(minDurationMs + 1); // +1 to approx. ceil of duration_ms
    }
  }
};

// this object copies frames from system memory to picture manager
// dynAtlas have 3 textures in ring for fast and cheap moving data from CPU to GPU
struct PictureManagerUpdater
{
  static int dynAtlasWidth;
  static int dynAtlasHeight;

  LottieRenderThread &renderThread;

  DynamicAtlasFencedTexUpdater atlas;
  LockedTexInfo readyForPm;

  PictureManagerUpdater(LottieRenderThread &r) : renderThread(r) {}

  void termAtlas() { atlas.term(); }

  void loadPending()
  {
    if (!atlas.tryLock())
      return;

    uploadReadyFramesToSysTex();
    copyCurrentFramesFromSysTex();
  }

  void copyCurrentFramesFromSysTex()
  {
    if (readyForPm.copy_rects.empty())
    {
      atlas.unlockCurrent();
      return;
    }

    auto unlockData = atlas.unlockCurrent();

#if DEBUG_LOTTIE_UPDATE
    debug("start update 0x%p", unlockData.tex);
#endif

    for (const auto &r : readyForPm.copy_rects)
    {
      TextureInfo tex_info;
      BaseTexture *dstTexture = acquire_managed_tex(r.tId);
      dstTexture->getinfo(tex_info);
      dstTexture->updateSubRegion(unlockData.tex, 0, r.cr.x0, r.cr.y0, 0, r.cr.w, r.cr.h, 1, 0, r.out_lt.x * tex_info.w,
        r.out_lt.y * tex_info.h, 0);
      d3d::resource_barrier({dstTexture, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      release_managed_tex(r.tId);
      release_managed_tex(r.tId); //== release reference acquired in uploadReadyFramesToSysTex()
#if DEBUG_LOTTIE_UPDATE
      debug("update frame (%s) from %d,%d", get_managed_texture_name(r.tId), r.cr.x0, r.cr.y0);
#endif
    }
#if DEBUG_LOTTIE_UPDATE
    for (const auto &r : readyForPm.copy_rects_d)
    {
      debug("path:%s dur:%d frame:%d", r.lottie, r.duration, r.frame);
    }
#endif

    readyForPm.copy_rects.clear();
#if DEBUG_LOTTIE_UPDATE
    readyForPm.copy_rects_d.clear();
#endif

    // we must set fence for used texture after all copy operations
    d3d::issue_event_query(unlockData.writeCompeletionFence);
  }

  void init() { atlas.init({dynAtlasWidth, dynAtlasHeight}, LottieAnim::LOTTIE_SURFACE_FMT); }

  void uploadReadyFramesToSysTex()
  {
    // prepared frames stored in readyFrames array now (in system memory)
    // but need move those to gpu memory with dyn atlas textures
    LottieReadyFrame readyFrame;
    while (renderThread.popReadyFrame(readyFrame))
    {
      // try to add frame into atlas update
      // if we can't - current update texture is full
      CopyRecTx crTx;
      if (!atlas.add(readyFrame.size.x, readyFrame.size.y, crTx.cr))
        break;

      // move frame from sys memory to texture
      atlas.fillData((uint8_t *)readyFrame.data.data(), readyFrame.size.x, readyFrame.size.y, crTx.cr.x0, crTx.cr.y0);
#if DEBUG_LOTTIE_UPDATE
      debug("%08X.makeFrame() to %d,%d", readyFrame.pid, crTx.cr.x0, crTx.cr.y0);
#endif

      // received texture id and size in uv from picture id, it need that execute copy action later
      crTx.tId = PictureManager::get_picture_data(readyFrame.pid, crTx.out_lt, crTx.out_rb);

      // 0 if texture not found, because PM was reset or discarded, not need this animation more
      if (auto dstTexture = acquire_managed_tex(crTx.tId))
      {
        readyForPm.copy_rects.push_back(crTx);
#if DEBUG_LOTTIE_UPDATE
        readyForPm.copy_rects_d.push_back({readyFrame.lottie, readyFrame.frame_dbg, readyFrame.duration_ms});
#endif
      }
      else
      {
        // PM not contains information about texture with current TEXTURE_ID,
        // possible it not a error, but we cant works with this texture anymore, so just remove it
        renderThread.addCommand(LottieRenderCommand::DISCARD_PID, DataBlock::emptyBlock, readyFrame.pid);
      }
    }
  }
};

int PictureManagerUpdater::dynAtlasWidth = 512;
int PictureManagerUpdater::dynAtlasHeight = 512;

struct LottieAnimationRenderer
{
  LottieRenderThread renderThread;
  PictureManagerUpdater pmUpdate;

  WinCritSec animationsPresentMutex;
  eastl::vector_map<uint32_t, LottieAnimDesc> animationsPresent;

  bool match(const DataBlock &pic_props, int &out_w, int &out_h)
  {
    const char *path = pic_props.getStr("lottie");
    if (!path || 0 == *path)
      return false;

    WinAutoLock lock(animationsPresentMutex);
    uint32_t propsHash = LottieAnim::getPropsHash(pic_props);
    auto it = animationsPresent.find(propsHash);
    if (it == animationsPresent.end())
    {
      IPoint2 prefferedSize = LottieAnim::getSizeFromBlk(pic_props);
      LottieAnimDesc animDesc;
      animDesc.pid = BAD_PICTUREID;
      animDesc.size = prefferedSize;
      animationsPresent.insert({propsHash, animDesc});

      if (!renderThread.isThreadStarted()) // Start thread on first usage
        renderThread.start();

      renderThread.addCommand(LottieRenderCommand::ADD_CONFIG, pic_props);

      out_w = prefferedSize.x;
      out_h = prefferedSize.y;

      return true;
    }

    out_w = it->second.size.x;
    out_h = it->second.size.y;

    return true;
  }

  bool render(Texture *to, int x, int y, int dstw, int dsth, const DataBlock &info, PICTUREID pid)
  {
    renderThread.addCommand(LottieRenderCommand::SETUP_PID, info, pid);

    WinAutoLock lock(animationsPresentMutex);
    auto it = animationsPresent.find(LottieAnim::getPropsHash(info));
    if (it != animationsPresent.end())
      it->second.pid = pid;

    return true;
  }

  void clear()
  {
    renderThread.terminate(true);
    pmUpdate.termAtlas();
  }

  void play(PICTUREID pid, bool play) { renderThread.addCommand(LottieRenderCommand::SETUP_PLAY, DataBlock::emptyBlock, pid, play); }

  void discard(PICTUREID pid)
  {
    renderThread.addCommand(LottieRenderCommand::DISCARD_PID, DataBlock::emptyBlock, pid);

    {
      WinAutoLock lock(animationsPresentMutex);
      auto it = eastl::find_if(animationsPresent.begin(), animationsPresent.end(), [pid](auto &a) { return a.second.pid == pid; });
      if (it != animationsPresent.end())
        animationsPresent.erase(it);
    }
  }

  void updatePerFrame() { pmUpdate.loadPending(); }

  LottieAnimationRenderer() : pmUpdate(renderThread) { pmUpdate.init(); }
};

struct LottieRenderFactoryPF final : public PictureManager::PictureRenderFactory
{
  InitOnDemand<LottieAnimationRenderer, false> ctx;

  void registered() override { ctx.demandInit(); }
  void clearPendReq() override {}
  void discard(PICTUREID pId)
  {
    if (ctx)
      ctx->discard(pId);
  }

  void play(PICTUREID pid, bool play)
  {
    if (ctx)
      ctx->play(pid, play);
  }

  void updatePerFrame() override
  {
    if (ctx)
      ctx->updatePerFrame();
  }
  void unregistered() override
  {
    if (ctx)
      ctx->clear();
    ctx.demandDestroy();
  }
  bool match(const DataBlock &pic_props, int &out_w, int &out_h) override { return !ctx || ctx->match(pic_props, out_w, out_h); }
  bool render(Texture *to, int x, int y, int w, int h, const DataBlock &info, PICTUREID pid) override
  {
    return !ctx || ctx->render(to, x, y, w, h, info, pid);
  }
};

static LottieRenderFactoryPF render_lottie_animation_factory;

void register_lottie_tex_load_factory(unsigned int atlas_w, unsigned int atlas_h)
{
  PictureManagerUpdater::dynAtlasWidth = atlas_w;
  PictureManagerUpdater::dynAtlasHeight = atlas_h;
  debug("Register lottie animations factory at %p", (intptr_t)&render_lottie_animation_factory);
}

void lottie_discard_animation(PICTUREID pid) { render_lottie_animation_factory.discard(pid); }

void lottie_play_animation(PICTUREID pid, bool play) { render_lottie_animation_factory.play(pid, play); }