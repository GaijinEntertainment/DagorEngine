// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <videoPlayer/dag_videoPlayer.h>
#include <theora/theora.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_tex3d.h>
#include <image/dag_texPixel.h>
#include <perfMon/dag_cpuFreq.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <math/integer/dag_IPoint2.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_asyncRead.h>
#include <osApiWrappers/dag_miscApi.h>
#include <stdio.h>
#if _TARGET_PC
#include "data_pc.h"
#else
#include "data_min.h"
#include <drv/3d/dag_platform.h>
#endif


static Tab<class OggVideoPlayer *> video_players(midmem);
static Tab<int> player_priorities(tmpmem);


class OggVideoPlayer : public IGenVideoPlayer, public VideoPlaybackData
{
  ogg_sync_state oy;
  ogg_page og;
  ogg_stream_state to;
  theora_info ti;
  theora_comment tc;
  theora_state td;
  ogg_packet op;
  int theora_p;
  int stateflag;
  int videobuf_ready;
  ogg_int64_t videobuf_granulepos;
  double videobuf_time;

  int64_t ref_t;
  int basePosMsec;
  int frame_time, last_frame_time;
  int movieDurMsec, tmpSumMovieDurUsec;
  //*timing*/int decode_start_time;

  int frameW, frameH;
  bool isPlayed, frameIsSkipped, autoRewind, isEOFReached;
  bool canRewindNow;
  int lateFrameCount;
  int lastRdPos;

  struct FileIo
  {
    file_ptr_t handle;
    int areq;
    char *buf;
    int bufSz, fileSz;
    int cPos;
    int pendReadSz, rdBufOfs;
    int cBufPos, cBufLeft;

    bool open(const char *fname)
    {
      const char *realName = ::df_get_real_name(fname);
      handle = realName ? ::dfa_open_for_read(realName, true) : NULL;
      if (!handle)
      {
        DEBUG_CTX("cannot open %s(%s)", fname, realName ? realName : "");
        return false;
      }

      int chunkSize = ::dfa_chunk_size(realName);
      fileSz = ::dfa_file_length(handle);

      bufSz = 2 << 20;
      if (bufSz > fileSz / 2)
        bufSz = (fileSz / 2 + chunkSize - 1) / chunkSize * chunkSize;
      if (bufSz < chunkSize)
        bufSz = chunkSize;

      buf = (char *)tmpmem->allocAligned(bufSz * 2, chunkSize);
      areq = ::dfa_alloc_asyncdata();
      return true;
    }

    void close()
    {
      if (!handle)
        return;

      ::dfa_close(handle);
      if (pendReadSz)
        finishRead();
      ::dfa_free_asyncdata(areq);

      if (buf)
      {
        tmpmem->freeAligned(buf);
        buf = NULL;
      }

      memset(this, 0, sizeof(*this));
      areq = -1;
    }

    int finishRead(int ms = 0)
    {
      int bytes = 0;
      while (!::dfa_check_complete(areq, &bytes))
        ::sleep_msec(ms);
      return bytes;
    }

    void rewind()
    {
      if (!handle)
        return;
      cPos = 0;
      cBufPos = cBufLeft = 0;
      if (pendReadSz)
        finishRead();
      pendReadSz = 0;
      rdBufOfs = 0;
    }
  } file;

public:
  OggVideoPlayer(int q_depth)
  {
    memset(&oy, 0, sizeof(oy));
    memset(&og, 0, sizeof(og));
    memset(&to, 0, sizeof(to));
    memset(&ti, 0, sizeof(ti));
    memset(&tc, 0, sizeof(tc));
    memset(&td, 0, sizeof(td));
    memset(&op, 0, sizeof(op));
    theora_p = 0;
    stateflag = 0;
    videobuf_ready = 0;
    videobuf_granulepos = -1;
    videobuf_time = 0;
    isPlayed = false;
    frameIsSkipped = false;
    lateFrameCount = 0;
    frameW = 0;
    frameH = 0;
    isEOFReached = false;
    autoRewind = false;
    canRewindNow = false;
    basePosMsec = 0;
    movieDurMsec = -1;
    tmpSumMovieDurUsec = 0;

    memset(&file, 0, sizeof(file));
    file.areq = -1;

    initBuffers(q_depth);
    lastRdPos = -1;
    measure_cpu_freq();
  }

  ~OggVideoPlayer()
  {
    close();
    frameW = 0;
    frameH = 0;
    termVideoBuffers();
    termBuffers();
    lastRdPos = -1;

    for (int i = 0; i < video_players.size(); i++)
    {
      if (video_players[i] == this)
      {
        erase_items(video_players, i, 1);
        DEBUG_CTX("removed ogg player, count = %d", video_players.size());
        break;
      }
    }
  }

  bool openFile(const char *fname)
  {
    if (!file.open(fname))
      return false;

    /* start up Ogg stream synchronization layer */
    ogg_sync_init(&oy);

    if (!initDecoder())
    {
      DEBUG_CTX("cannot init decoder for %s", fname);
      file.close();
      theora_p = 0;
      return false;
    }

    video_players.push_back(this);
    DEBUG_CTX("added ogg player for '%s', count = %d", fname, video_players.size());
    return true;
  }

  bool openMem(dag::ConstSpan<char> data)
  {
    /* start up Ogg stream synchronization layer */
    ogg_sync_init(&oy);
    oy.data = (unsigned char *)data.data();
    oy.storage = data_size(data);

    if (!initDecoder())
    {
      DEBUG_CTX("cannot init decoder to read OGG from mem");
      file.close();
      theora_p = 0;
      return false;
    }

    video_players.push_back(this);
    DEBUG_CTX("added ogg player for mem %p, sz=%u; count = %d", data.data(), data_size(data), video_players.size());
    return true;
  }

  void close()
  {
    if (!file.handle)
      ogg_sync_init(&oy);
    else
      file.close();
    termDecoder();
    vBuf.wrPos = vBuf.rdPos = (vBuf.rdPos + 1) % vBuf.depth;
    vBuf.used = 0;
    lastRdPos = -1;
  }

  // IGenVideoPlayer interface implementation
  virtual IGenVideoPlayer *reopenFile(const char *fname)
  {
    close();

    if (!file.open(fname))
    {
      destroy();
      return NULL;
    }
    if (!initDecoder())
    {
      DEBUG_CTX("cannot init decoder for %s", fname);
      destroy();
      return NULL;
    }
    return this;
  }
  virtual IGenVideoPlayer *reopenMem(dag::ConstSpan<char> data)
  {
    close();

    oy.data = (unsigned char *)data.data();
    oy.storage = data_size(data);
    if (!initDecoder())
    {
      DEBUG_CTX("cannot init decoder to read OGG from mem");
      destroy();
      return NULL;
    }
    return this;
  }

  virtual int getPosition()
  {
    int pos = basePosMsec + unsigned(get_time_usec_qpc(ref_t) / 1000);
    return movieDurMsec > 0 ? pos % movieDurMsec : pos;
  }
  virtual int getDuration() { return movieDurMsec; }

  virtual void start(int prebuf_delay_ms)
  {
    if (!isPlayed)
    {
      isPlayed = true;
      ref_t = ref_time_ticks_qpc();
      last_frame_time = prebuf_delay_ms * 1000;

      if (!vBuf.used)
      {
        while (vBuf.used < 1 && !isFinished())
          advance(5000);

        last_frame_time = prebuf_delay_ms * 1000;
        for (int i = 0; i < vBuf.used; i++)
        {
          vBuf.getRd(i).time = last_frame_time;
          last_frame_time += frame_time;
        }
        ref_t = ref_time_ticks_qpc();
      }

      //*timing*/decode_start_time = get_time_usec_qpc(ref_t);
    }
  }
  virtual void stop(bool reset)
  {
    isPlayed = false;
    if (reset)
    {
      vBuf.wrPos = vBuf.rdPos = (vBuf.rdPos + 1) % vBuf.depth;
      vBuf.used = 0;
      lastRdPos = -1;
      basePosMsec = 0;
    }
  }
  virtual void destroy() { delete this; }
  virtual void advance(int max_usec) { mainLoop(max_usec); }

  virtual bool getFrame(TEXTUREID &idY, TEXTUREID &idU, TEXTUREID &idV, d3d::SamplerHandle &smp)
  {
    smp = sampler;
    if (!vBuf.used)
    {
    no_new_frames:
      if (lastRdPos >= 0)
      {
        // debug("can show frame %d", lastRdPos);
#if _TARGET_PC
        if (vBuf.buf[0].texIdY == BAD_TEXTUREID)
        {
          idY = useVideo[currentVideoFrame].texIdY;
          idU = useVideo[currentVideoFrame].texIdU;
          idV = useVideo[currentVideoFrame].texIdV;
        }
        else
#endif
        {
          idY = vBuf.buf[lastRdPos].texIdY;
          idU = vBuf.buf[lastRdPos].texIdU;
          idV = vBuf.buf[lastRdPos].texIdV;
        }
        return true;
      }
      return false;
    }

    // debug ( "render: ct=%9d  framet=%9d", get_time_usec_qpc(ref_t), vBuf.getRd().time );

    // if ( get_time_usec_qpc(ref_t)+frame_time/2 > vBuf.getRd().time && vBuf.used > 0 )
    int targetTime = get_time_usec_qpc(ref_t) + frame_time / 2;
    while (targetTime > vBuf.getRd().time && vBuf.used > 1)
      vBuf.incRd();

    if (!vBuf.used)
      goto no_new_frames;

    lastRdPos = vBuf.rdPos;
    getCurrentTex(idY, idU, idV);
    return true;
  }
  virtual void onCurrentFrameDispatched() override
  {
    if (vBuf.buf[0].texIdY != BAD_TEXTUREID && lastRdPos >= 0)
    {
      auto &ev = vBuf.buf[lastRdPos].ev;
      if (!ev)
        ev = d3d::create_event_query(); // Init on demand to ensure that it's called from render (main) thread
      d3d::issue_event_query(ev);
    }
  }
  virtual bool renderFrame(const IPoint2 *, const IPoint2 *) { return false; }

  virtual void setAutoRewind(bool auto_rewind) { autoRewind = auto_rewind; }
  virtual void rewind()
  {
    if (!canRewindNow)
    {
      debug("cant rewind now, skipped");
      return;
    }

    debug("rewind, vBuf.used=%d", vBuf.used);
    if (file.handle)
      file.rewind();
    else
      oy.fill = oy.returned = 0;

    videobuf_ready = 0;
    canRewindNow = false;
    ogg_stream_reset(&to);
    isEOFReached = false;
  }

  virtual IPoint2 getFrameSize() { return IPoint2(frameW, frameH); }

  virtual bool isFinished() { return !theora_p || !isPlayed; }

  virtual void beforeResetDevice()
  {
    for (int bufNo = 0; bufNo < vBuf.getDepth(); bufNo++)
    {
      if (vBuf.buf[bufNo].ev)
      {
        d3d::release_event_query(vBuf.buf[bufNo].ev);
        vBuf.buf[bufNo].ev = NULL;
      }
    }
  }

  virtual void afterResetDevice()
  {
    for (int bufNo = 0; bufNo < vBuf.getDepth(); bufNo++)
    {
      if (!vBuf.buf[bufNo].ev)
        vBuf.buf[bufNo].ev = d3d::create_event_query();
    }
  }


private:
  bool initDecoder()
  {
    basePosMsec = 0;
    movieDurMsec = -1;
    tmpSumMovieDurUsec = 0;
    ref_t = ref_time_ticks_qpc();

    /*
       Ok, Ogg parsing. The idea here is we have a bitstream
       that is made up of Ogg pages. The libogg sync layer will
       find them for us. There may be pages from several logical
       streams interleaved; we find the first theora stream and
       ignore any others.

       Then we pass the pages for our stream to the libogg stream
       layer which assembles our original set of packets out of
       them. It's the packets that libtheora actually knows how
       to handle.
    */

    /* init supporting Theora structures needed in header parsing */
    theora_comment_init(&tc);
    theora_info_init(&ti);

    /* Ogg file open; parse the headers */

    /* Vorbis and Theora both depend on some initial header packets
       for decoder setup and initialization. We retrieve these first
       before entering the main decode loop. */

    /* Only interested in Theora streams */
    while (!stateflag)
    {
      int ret = buffer_data();
      if (ret == 0)
        break;

      while (ogg_sync_pageout(&oy, &og) > 0)
      {
        ogg_stream_state test;

        /* is this a mandated initial header? If not, stop parsing */
        if (!ogg_page_bos(&og))
        {
          /* don't leak the page; get it into the appropriate stream */
          queue_page();
          stateflag = 1;
          break;
        }

        ogg_stream_init(&test, ogg_page_serialno(&og));
        ogg_stream_pagein(&test, &og);
        ogg_stream_packetout(&test, &op);

        /* identify the codec: try theora */
        if (!theora_p && theora_decode_header(&ti, &tc, &op) >= 0)
        {
          /* it is theora -- save this stream state */
          memcpy(&to, &test, sizeof(test));
          theora_p = 1;
        }
        else
        {
          /* whatever it is, we don't care about it */
          ogg_stream_clear(&test);
        }
      }
      /* fall through to non-initial page parsing */
    }

    /* we're expecting more header packets. */
    while (theora_p && theora_p < 3)
    {
      int ret;

      /* look for further theora headers */
      while (theora_p && (theora_p < 3) && (ret = ogg_stream_packetout(&to, &op)) != 0)
      {
        if (ret < 0)
        {
          DEBUG_CTX("Error parsing Theora stream headers; corrupt stream?\n");
          return false;
        }
        if (theora_decode_header(&ti, &tc, &op))
        {
          DEBUG_CTX("Error parsing Theora stream headers; corrupt stream?\n");
          return false;
        }
        theora_p++;
        if (theora_p == 3)
          break;
      }


      /* The header pages/packets will arrive before anything else we
         care about, or the stream is not obeying spec */

      if (ogg_sync_pageout(&oy, &og) > 0)
      {
        queue_page(); /* demux into the stream state */
      }
      else
      {
        if (buffer_data() == 0)
        { /* need more data */
          DEBUG_CTX("End of file while searching for codec headers.\n");
          return false;
        }
      }
    }

    /* Now we have all the required headers. initialize the decoder. */
    if (theora_p)
    {
      theora_decode_init(&td, &ti);

      frame_time = int(1000000 / ((double)ti.fps_numerator / ti.fps_denominator));

      DEBUG_CTX("Ogg logical stream %lX is Theora %ux%u %.02f fps video (%d usec per frame)\n"
                "Encoded frame content is %ux%u with %ux%u offset; %d bitrate, %d quality, %d quick",
        to.serialno, ti.width, ti.height, (float)ti.fps_numerator / ti.fps_denominator, frame_time, ti.frame_width, ti.frame_height,
        ti.offset_x, ti.offset_y, ti.target_bitrate, ti.quality, ti.quick_p);
    }
    else
    {
      /* tear down the partial theora setup */
      theora_info_clear(&ti);
      theora_comment_clear(&tc);
      return false;
    }

    /* open video */
    if (frameW != ti.frame_width || frameH != ti.frame_height)
    {
      if (frameW && frameH)
      {
        DEBUG_CTX("need to recreate buffers, frame size changed %dx%d -> %ux%u", frameW, frameH, ti.frame_width, ti.frame_height);
        termVideoBuffers();
        frameW = frameH = 0;
      }

      lastRdPos = -1;
      frameW = ti.frame_width;
      frameH = ti.frame_height;
      if (!initVideoBuffers(ti.frame_width, ti.frame_height))
      {
        theora_info_clear(&ti);
        theora_comment_clear(&tc);
        theora_p = 0;
        return false;
      }
    }

    stateflag = 0; /* playback has not begun */
    /* queue any remaining pages from data we buffered but that did not
        contain headers */
    while (ogg_sync_pageout(&oy, &og) > 0)
      queue_page();

    canRewindNow = false;
    return true;
  }
  void termDecoder()
  {
    /* end of decoder loop -- close everything */

    ogg_stream_clear(&to);
    theora_clear(&td);
    theora_comment_clear(&tc);
    theora_info_clear(&ti);
    ogg_sync_clear(&oy);
    memset(&op, 0, sizeof(op));
    memset(&og, 0, sizeof(og));

    theora_p = 0;
    stateflag = 0;
    videobuf_ready = 0;
    videobuf_granulepos = -1;
    videobuf_time = 0;
    isPlayed = false;
    frameIsSkipped = false;
    lateFrameCount = 0;
    isEOFReached = false;
  }


  void mainLoop(int max_usec)
  {
    /* Finally the main decode loop.

       It's one Theora packet per frame, so this is pretty
       straightforward if we're not trying to maintain sync
       with other multiplexed streams.

       the videobuf_ready flag is used to maintain the input
       buffer in the libogg stream state. If there's no output
       frame available at the end of the decode step, we must
       need more input data. We could simplify this by just
       using the return code on ogg_page_packetout(), but the
       flag system extends easily to the case were you care
       about more than one multiplexed stream (like with audio
       playback). In that case, just maintain a flag for each
       decoder you care about, and pull data when any one of
       them stalls.

       videobuf_time holds the presentation time of the currently
       buffered video frame. We ignore this value.
    */
    if (!isPlayed || !theora_p)
      return;

    int end_time = get_time_usec_qpc(ref_t);
    if (end_time > 120000000)
      end_time = rebaseTimer(end_time);

    int targetTime = end_time + frame_time / 2;
    while (targetTime > vBuf.getRd().time && vBuf.used > 0)
      vBuf.incRd();
    end_time += max_usec;

    do
    {
      while (!videobuf_ready)
      {
        /* theora is one in, one out... */
        if (ogg_stream_packetout(&to, &op) > 0)
        {
          if (!frameIsSkipped)
          {
            // decode video frame
            theora_decode_packetin(&td, &op);
            videobuf_granulepos = td.granulepos;
            videobuf_time = theora_granule_time(&td, videobuf_granulepos);
          }
          videobuf_ready = 1;

          //*timing*/decode_start_time = get_time_usec_qpc(ref_t) - decode_start_time;
          //*timing*/DEBUG_CTX("frame decode: %d usec", decode_start_time);
        }
        else
          break;

        if (get_time_usec_qpc(ref_t) >= end_time)
        {
          //*timing*/DEBUG_CTX("timeout: end_time=%d usec", end_time);
          return;
        }
      }

      if (get_time_usec_qpc(ref_t) >= end_time)
      {
        //*timing*/DEBUG_CTX("timeout: end_time=%d usec", end_time);
        return;
      }

      if (isEOF() && !file.pendReadSz)
      {
        if (movieDurMsec == -1)
        {
          movieDurMsec = tmpSumMovieDurUsec / 1000;
          debug("now movie duration known: %d msec", movieDurMsec);
        }
        if (autoRewind)
        {
          rewind();
          return;
        }
        else if (!videobuf_ready && vBuf.used < 1)
        {
          isPlayed = false;
          DEBUG_CTX("stop playing");
          return;
        }
      }

      if (!videobuf_ready && !isEOF())
      {
        /* no data yet for somebody.  Grab another page */
        buffer_data();
        while (ogg_sync_pageout(&oy, &og) > 0)
          queue_page();
        continue;
      }

      /* dumpvideo frame, and get new one */
      if (vBuf.bufLeft() < 2)
        return;
      if (!d3d::get_event_query_status(vBuf.buf[vBuf.wrPos].ev, false))
        return;

      int ct = get_time_usec_qpc(ref_t);
      int t = last_frame_time + frame_time;
      tmpSumMovieDurUsec += frame_time;

      if (isEOF())
        ;
      else if (lateFrameCount >= 4)
      {
        int delta = (ct + 80000 - t + frame_time - 1) / frame_time * frame_time;
        ref_t = rel_ref_time_ticks(ref_t, delta);
        ct = get_time_usec_qpc(ref_t);
        lateFrameCount = 0;
#if !_TARGET_IOS
        debug("late frames detected, instant time increment=%d ms", delta / 1000);
#endif
      }
      else if (t < ct + frame_time / 2 + 1000)
        lateFrameCount++;
      else
        lateFrameCount = 0;

      if (videobuf_ready || !isEOF())
      {
        if (!frameIsSkipped && !lateFrameCount)
          enqueue_frame(t);
#if !_TARGET_IOS
        else
          debug("skip frame #%d", t / frame_time);
#endif
      }

      last_frame_time = t;

      if (t > ct - 40000)
        frameIsSkipped = false;

      //*timing*/decode_start_time = get_time_usec_qpc(ref_t);

      videobuf_ready = 0;

    } while (get_time_usec_qpc(ref_t) < end_time);
  }

private:
  int buffer_data()
  {
    if (!file.handle)
    {
      G_ASSERT(oy.data);
      if (oy.fill == oy.storage)
        isEOFReached = true;

      int nfill = (oy.fill + (128 << 10)) & 0xFFFF0000;

      if (nfill > oy.storage)
        nfill = oy.storage;

      int inc = nfill - oy.fill;
      oy.fill = nfill;
      return inc;
    }

    const int buffer_amount = 256 << 10;
    if (!file.cBufLeft && !isEOFReached)
    {
      file.cBufLeft = readData();
      file.cBufPos = 0;
    }
    int readBytes = buffer_amount;
    if (file.cBufLeft < readBytes)
      readBytes = file.cBufLeft;

    char *buffer = ogg_sync_buffer(&oy, readBytes);
    memcpy(buffer, file.buf + file.rdBufOfs + file.cBufPos, readBytes);
    ogg_sync_wrote(&oy, readBytes);

    file.cBufPos += readBytes;
    file.cBufLeft -= readBytes;
    return readBytes;
  }

  int readData()
  {
    if (!file.pendReadSz)
    {
      file.rdBufOfs = file.bufSz;
      if (file.cPos >= file.fileSz || !::dfa_read_async(file.handle, file.areq, file.cPos, file.buf, file.bufSz))
      {
        isEOFReached = true;
        return 0;
      }
      file.pendReadSz = file.bufSz;
    }

    int bytes = file.finishRead(1);
    int wrBufOfs = file.rdBufOfs;
    file.rdBufOfs = file.bufSz - file.rdBufOfs;

    file.cPos += bytes;
    if (bytes == file.pendReadSz)
    {
      if (!::dfa_read_async(file.handle, file.areq, file.cPos, file.buf + wrBufOfs, file.bufSz))
        isEOFReached = true;
      file.pendReadSz = file.bufSz;
    }
    else
      file.pendReadSz = 0;
    return bytes;
  }

  int queue_page()
  {
    if (theora_p)
      ogg_stream_pagein(&to, &og);
    return 0;
  }
  bool isEOF() { return isEOFReached; }

  void enqueue_frame(int t)
  {
    Texture *tex;

    // detect target tex
    if (vBuf.bufLeft() <= 1)
    {
      logerr("skip: vBuf.bufLeft()=%d wrPos=%d rdPos=%d", vBuf.bufLeft(), vBuf.wrPos, vBuf.rdPos);
      return;
    }
    if (lastRdPos == vBuf.wrPos)
      lastRdPos = -1;

    vBuf.getWr().time = t;
    tex = vBuf.getWr().texY;

    // receive yuv pointers
    yuv_buffer yuv;
    theora_decode_YUVout(&td, &yuv);

    // copy pixels
    unsigned char *ptr;
    int stride;
    int fh2 = frameH / 2, fw2 = frameW / 2;

    //*timing*/int t_st = get_time_usec_qpc(ref_t);
    if (!tex->lockimgEx(&ptr, stride, 0, TEXLOCK_WRITE | TEXLOCK_RAWDATA))
    {
      tex->unlockimg();
      return;
    }
    uint8_t *y_ptr = yuv.y;
    if (stride == yuv.y_stride && stride == frameW)
      memcpy(ptr, y_ptr, frameW * frameH);
    else
      for (int y = 0; y < frameH; y++, ptr += stride, y_ptr += yuv.y_stride)
        memcpy(ptr, y_ptr, frameW);
    tex->unlockimg();

    if (!vBuf.getWr().texU->lockimgEx(&ptr, stride, 0, TEXLOCK_WRITE | TEXLOCK_RAWDATA))
    {
      tex->unlockimg();
      return;
    }
    uint8_t *u_ptr = yuv.u;
    for (int y = 0; y < fh2; y++, ptr += stride, u_ptr += yuv.uv_stride)
      memcpy(ptr, u_ptr, fw2);
    vBuf.getWr().texU->unlockimg();

    if (!vBuf.getWr().texV->lockimgEx(&ptr, stride, 0, TEXLOCK_WRITE | TEXLOCK_RAWDATA))
    {
      tex->unlockimg();
      return;
    }
    uint8_t *v_ptr = yuv.v;
    for (int y = 0; y < fh2; y++, ptr += stride, v_ptr += yuv.uv_stride)
      memcpy(ptr, v_ptr, fw2);
    vBuf.getWr().texV->unlockimg();
    //*timing*/t_st = get_time_usec_qpc(ref_t) - t_st;
    //*timing*/debug ( "converted: %d usec", t_st);

    vBuf.incWr();
    canRewindNow = true;
  }
  int rebaseTimer(int t_usec)
  {
    if (vBuf.used && vBuf.getRd().time < t_usec)
      t_usec = vBuf.getRd().time;
    if (last_frame_time < t_usec)
      t_usec = last_frame_time;

    if (t_usec < 1000000 + frame_time * vBuf.depth)
      return t_usec;

    t_usec = ((t_usec - frame_time * vBuf.depth) / 1000000) * 1000000;
    debug("rebase timer (%d usec)", t_usec);

    for (int i = 0; i < vBuf.used; i++)
      vBuf.getRd(i).time -= t_usec;
    basePosMsec += t_usec / 1000;
    last_frame_time -= t_usec;

    ref_t = rel_ref_time_ticks(ref_t, t_usec);
    return get_time_usec_qpc(ref_t);
  }

  friend void IGenVideoPlayer::advanceActivePlayers(int);
};


// maker for Ogg Theora decoder based player
IGenVideoPlayer *IGenVideoPlayer::create_ogg_video_player(const char *fname, int buf_f)
{
  OggVideoPlayer *vp = new (midmem) OggVideoPlayer(buf_f);
  if (vp->openFile(fname))
    return vp;
  vp->destroy();
  return NULL;
}

IGenVideoPlayer *IGenVideoPlayer::create_ogg_video_player_mem(dag::ConstSpan<char> data, int buf_f)
{
  OggVideoPlayer *vp = new (midmem) OggVideoPlayer(buf_f);
  if (vp->openMem(data))
    return vp;
  vp->destroy();
  return NULL;
}


void IGenVideoPlayer::advanceActivePlayers(int max_usec)
{
  int num = video_players.size();

  if (!num)
    return;

  int totalPriority = 0;

  player_priorities.resize(num);

  for (int i = 0; i < num; i++)
  {
    OggVideoPlayer *player = video_players[i];
    if (!player->isPlayed)
      player_priorities[i] = 0;
    else
    {
      int p = ((player->frameW * player->frameH) >> 8) + 1;
      player_priorities[i] = p;
      totalPriority += p;
    }
  }

  for (int i = 0; i < video_players.size(); i++)
    if (player_priorities[i] > 0)
      video_players[i]->advance(max_usec * player_priorities[i] / totalPriority);
}
