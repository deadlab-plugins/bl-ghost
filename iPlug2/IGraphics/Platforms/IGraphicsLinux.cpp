/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <wdlutf8.h>

#include "IPlugParameter.h"
#include "IGraphicsLinux.h"
#include "IPlugPaths.h"
#include <unistd.h>
#include <sys/wait.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xfixes.h>

#ifdef OS_LINUX
  #ifdef IGRAPHICS_GL
    #include "glad.c"
    #include <glad/glad_glx.h>
  #endif
  #include <fontconfig/fontconfig.h>
#endif

#define NOT_IMPLEMENTED printf("%s: not implemented\n", __FUNCTION__);

// Use another format for file filters
// to be able to use all the filters at the same time
// (and not choose for example only ".wav", then only ".aif" etc.)
#define BL_FILE_FILTER_PATCH 1

using namespace iplug;
using namespace igraphics;

#define IPLUG_TIMER_ID 2

class IGraphicsLinux::Font : public PlatformFont
{
public:
  Font(WDL_String &fileName) 
  : PlatformFont(false)
  , mFileName(fileName)
  {
    mFontData.Resize(0);
  }

  Font(WDL_String &fontID, const void* pData, int dataSize) 
  : PlatformFont(false)
  , mFileName(fontID)
  {
    mFontData.Set((const uint8_t*)pData, dataSize);
  }

  IFontDataPtr GetFontData() override;

private:
  WDL_String mFileName;
  WDL_TypedBuf<uint8_t> mFontData;
};

/** Run a child process with some input to stdin, and retrieve its stdout and exit status.
 * @param command Command-line to be passed to /bin/sh
 * @param subStdout Stores the stdout of the subprocess
 * @param subStdin The contents of stdin for the subprocess (may be empty)
 * @param exitStatus Output, exit status of the child process
 * @return 0 on success, a negative value on failure */
static int RunSubprocess(char* command, WDL_String& subStdout, const WDL_String& subStdin, int* exitStatus)
{
  int pipeOut[2];
  int pipeIn[2];
  pid_t cpid;

  auto closePipeOut = [&]() {
    close(pipeOut[0]);
    close(pipeOut[1]);
  };
  auto closePipeIn = [&]() {
    close(pipeIn[0]);
    close(pipeIn[1]);
  };

  if (pipe(pipeOut) == -1)
  {
    return -1;
  }

  if (pipe(pipeIn) == -1)
  {
    closePipeOut();
    return -2;
  }

  cpid = fork();
  if (cpid == -1)
  {
    closePipeOut();
    closePipeIn();
    return -3;
  }

  if (cpid == 0)
  {
    // Child process
    // Close unneeded pipes
    close(pipeIn[1]);
    close(pipeOut[0]);
    // Replace stdout/stderr and stdin
    dup2(pipeOut[1], STDOUT_FILENO);
    dup2(pipeOut[1], STDERR_FILENO);
    dup2(pipeIn[0], STDIN_FILENO);
    // Replace the child process with the new process
    WDL_String arg0 { "/bin/sh" };
    WDL_String arg1 { "-c" };
    char* args[] = { arg0.Get(), arg1.Get(), command, nullptr };
    int status = execv("/bin/sh", args);
    close(pipeIn[0]);
    close(pipeOut[1]);
    exit(status);
  }
  else 
  {
    // Parent process

    // Close unneeded pipes
    close(pipeIn[0]);
    close(pipeOut[1]);

    // We write all contents to stdin and read from stdout until it's empty.
    ssize_t written = 0;
    while (written < subStdin.GetLength())
    {
      write(pipeIn[1], subStdin.Get() + written, subStdin.GetLength() - written);
    }

    int status;
    if (waitpid(cpid, &status, 0) == -1)
    {
      closePipeOut();
      closePipeIn();
      return -4;
    }
    *exitStatus = WEXITSTATUS(status);

    WDL_HeapBuf hb;
    hb.Resize(4096);
    while (true)
    {
      ssize_t sz = read(pipeOut[0], hb.Get(), hb.GetSize());
      if (sz == 0)
      {
        break;
      }
      subStdout.Append((const char*)hb.Get(), sz);
    }

    closePipeOut();
    closePipeIn();
  }
  return 0;
}

static uint64_t GetTimeMs()
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t);
  return (t.tv_sec * 1000) + (t.tv_nsec / 1000000);
}

void IGraphicsLinux::Paint(IRECTList &rects)
{
  // #bluelab Do not redraw all each timen only dirty controls
  /*
    IRECT ir = {0, 0, static_cast<float>(WindowWidth()), static_cast<float>(WindowHeight())};
    IRECTList rects;
    rects.Add(ir.GetScaled(1.f / GetBackingPixelScale()));
  */
  
  void* ctx = xcbt_window_draw_begin(mPlugWnd);

  if (ctx)
  {
    Draw(rects);
    xcbt_window_draw_end(mPlugWnd);
  }
}

void IGraphicsLinux::DrawResize()
{
  void* ctx = xcbt_window_draw_begin(mPlugWnd);
  
  if (ctx)
  {
    IGRAPHICS_DRAW_CLASS::DrawResize();
    xcbt_window_draw_stop(mPlugWnd); // WARNING: in CAN BE reentrant!!! (f.e. it is called from SetScreenScale during initialization)
  }
  // WARNING: IPlug call it on resize, but at the end. When should we call Paint() ?
  // In Windows version "Update window" is called from PlatformResize, so BEFORE DrawResize...
}

inline IMouseInfo IGraphicsLinux::GetMouseInfo(int16_t x, int16_t y, int16_t state)
{
  IMouseInfo info;
  info.x = mCursorX = x / (GetDrawScale() * GetScreenScale());
  info.y = mCursorY = y / (GetDrawScale() * GetScreenScale());
  info.ms = IMouseMod((state & XCB_BUTTON_MASK_1), (state & XCB_BUTTON_MASK_3), // Note "2" is the middle button 
    (state & XCB_KEY_BUT_MASK_SHIFT), (state & XCB_KEY_BUT_MASK_CONTROL), (state & XCB_KEY_BUT_MASK_MOD_1) // shift, ctrl, alt
  );

  return info;
}

inline IMouseInfo IGraphicsLinux::GetMouseInfoDeltas(float& dX, float& dY, int16_t x, int16_t y, int16_t state)
{
  float oldX = mCursorX;
  float oldY = mCursorY;
  
  IMouseInfo info = GetMouseInfo(x, y, state);
  
  dX = info.x - oldX;
  dY = info.y - oldY;
  // dX = oldX - info.x;
  // dY = oldY - info.y;
  
  return info;
}

void IGraphicsLinux::TimerHandler(int timerID)
{
  // #bluelab
  // Windows may have just been destroyed
  if (mClosing || (mPlugWnd == NULL))
    return;

  mTimerProcessing = true;

  if (timerID == IPLUG_TIMER_ID)
  {
    IRECTList rects;
    if (IsDirty(rects))
    {
      Paint(rects);
      SetAllControlsClean();
    }
  
    // #bluelab
    int fps = FPS();
    int msec = (int)(1000.0/fps);
    
#if 1
    // Adjust, depending on the real time spent,
    // in order to stick more closer to the chosen fps
    const double timeStamp = GetTimestamp();
    const double timeDiff =  timeStamp- mPrevTimeStampPaint;
    mPrevTimeStampPaint = timeStamp;

    int diff = (int)(timeDiff*1000) - msec;
    if (diff > 0)
        mTimerDelay--;
    if (diff < 0)
        mTimerDelay++;
    if (mTimerDelay < 0)
        mTimerDelay = 0;
    
    msec = mTimerDelay;
#endif
        
    xcbt_timer_set(mX, IPLUG_TIMER_ID, msec, (xcbt_timer_cb) TimerHandlerProxy, this);
  }

  mTimerProcessing = false;
}

void IGraphicsLinux::WindowHandler(xcb_generic_event_t* evt)
{
  static struct timeval pt = {0}, ct;
  if (!evt)
  {
    mBaseWindowHandler(mPlugWnd, NULL, mBaseWindowData);
    mPlugWnd = nullptr;
  }
  else
  {
    switch(evt->response_type & ~0x80)
    {
      case XCB_EXPOSE:
      {
        xcb_expose_event_t *ee = (xcb_expose_event_t *)evt;

        if (!ee->count) // MAYBE: can collect and use invalidated areas
        {
          // #bluelab Repaint all (that was the default)
          IRECT ir = {0, 0, static_cast<float>(WindowWidth()), static_cast<float>(WindowHeight())};
          IRECTList rects;
          rects.Add(ir.GetScaled(1.f / GetBackingPixelScale()));
  
          Paint(rects);
        }
      }
      break;
      case XCB_BUTTON_PRESS:
      {
        xcb_button_press_event_t* bp = (xcb_button_press_event_t*) evt;

        bool btnLeft = bp->detail == 1;
        bool btnRight = bp->detail == 3;

        if (btnLeft) // check for double-click
        { 
          if (!mLastLeftClickStamp)
          {
            mLastLeftClickStamp = bp->time;
          } 
          else
          {
            if ((bp->time - mLastLeftClickStamp) < mDblClickTimeout)
            {
              IMouseInfo info = GetMouseInfo(bp->event_x, bp->event_y, bp->state | XCB_BUTTON_MASK_1); // convert button to state mask

              if (OnMouseDblClick(info.x, info.y, info.ms))
              {
                // TODO: SetCapture(hWnd);
              }
              mLastLeftClickStamp = 0;
              xcbt_flush(mX);
              break;
            }
            mLastLeftClickStamp = bp->time;
          }
        }
        else
        {
          mLastLeftClickStamp = 0;
        }

        // TODO: end parameter editing (if in progress, and return then)
        // TODO: set focus
        xcb_set_input_focus_checked(xcbt_conn(mX), XCB_INPUT_FOCUS_POINTER_ROOT, mPlugWnd->wnd, XCB_CURRENT_TIME);

        // TODO: detect double click
        
        // TODO: set capture (or after capture...) (but check other buttons first)
        if ((bp->detail == 1) || (bp->detail == 3)) // left/right
        { 
          uint16_t state = bp->state | (0x80<<bp->detail); // merge state before with pressed button
          IMouseInfo info = GetMouseInfo(bp->event_x, bp->event_y, state); // convert button to state mask
          std::vector<IMouseInfo> list{ info };
          OnMouseDown(list);
          RequestFocus();
        } 
        else if ((bp->detail == 4) || (bp->detail == 5)) // wheel
        { 
          IMouseInfo info = GetMouseInfo(bp->event_x, bp->event_y, bp->state);
          OnMouseWheel(info.x, info.y, info.ms, bp->detail == 4 ? 1. : -1);
        }
        xcbt_flush(mX);
        break;
      }
      case XCB_BUTTON_RELEASE:
      {
        xcb_button_release_event_t* br = (xcb_button_release_event_t*) evt;
        // TODO: release capture (but check other buttons first...)
        if ((br->detail == 1) || (br->detail == 3))
        { // we do not process other buttons, at least not yet
          uint16_t state = br->state & ~(0x80<<br->detail); // merge state before with released button
          IMouseInfo info = GetMouseInfo(br->event_x, br->event_y, state); // convert button to state mask
          std::vector<IMouseInfo> list{ info };
          OnMouseUp(list);
          RequestFocus();
        }
        xcbt_flush(mX);
        break;
      }
      case XCB_MOTION_NOTIFY:
      {
        xcb_motion_notify_event_t* mn = (xcb_motion_notify_event_t*) evt;
        if (mCursorLock && (float)mn->event_x == mMouseLockPos.x && (float)mn->event_y == mMouseLockPos.y)
        {
          break;
        }

        mLastLeftClickStamp = 0;
        if (mn->same_screen && (mn->event == xcbt_window_xwnd(mPlugWnd)))
        {
          // can use event_x/y
          if (!(mn->state & (XCB_BUTTON_MASK_1 | XCB_BUTTON_MASK_3))) // Not left/right drag
          {
            IMouseInfo info = GetMouseInfo(mn->event_x, mn->event_y, mn->state);
            OnMouseOver(info.x, info.y, info.ms);
          } 
          else 
          {
            // NOTE: this also updates mCursorX and mCursorY
            float dX, dY;
            IMouseInfo info = GetMouseInfoDeltas(dX, dY, mn->event_x, mn->event_y, mn->state);

            if (dX || dY)
            {
              info.dX = dX;
              info.dY = dY;
              std::vector<IMouseInfo> list{ info };

              OnMouseDrag(list);
              if (mCursorLock && (mCursorX != mMouseLockPos.x || mCursorY != mMouseLockPos.y))
              {
                MoveMouseCursor(mMouseLockPos.x, mMouseLockPos.y);
              }
            }
          }
        }
        xcbt_flush(mX);
        break;
      }
      case XCB_PROPERTY_NOTIFY:
      {
        xcb_property_notify_event_t* pn = (xcb_property_notify_event_t*) evt;
        if (pn->atom == XCBT_XEMBED_INFO(mX))
        {
          // TODO: check we really have to, but getting XEMBED_MAPPED and compare with current mapping status
          xcbt_window_map(mPlugWnd);
        }
        break;
      }
      case XCB_ENTER_NOTIFY:
      {
        RequestFocus();
        break;
      }
      case XCB_LEAVE_NOTIFY:
      {
        OnMouseOut();
        break;
      }
      case XCB_FOCUS_IN:
      {
        break;
      }
      case XCB_FOCUS_OUT:
      {
        break;
      }

      // #bluelab
      case XCB_KEY_PRESS:
      {
          xcb_key_press_event_t *ke = (xcb_key_press_event_t *)evt;

          float x = ke->event_x;
          float y = ke->event_y;

          xcb_keycode_t keycode = ke->detail;

          char utf8[7];
          uint32_t vk = ConvertKeyToVK(keycode, ke->state, utf8);
          
          IKeyPress key(utf8, vk);

          // Modifiers
          GetKeyModifiers(ke->state, &key.S, &key.C, &key.A);

#if 0 // DEBUG
          fprintf(stderr, "IKeyPress: %d %s s=%d a=%d c=%d\n",
                  key.VK, key.utf8, key.S, key.A, key.C);
#endif

          // #bluelab
          bool handled = false;
          IGEditorDelegate * delegate = GetDelegate();
          if (delegate != NULL)
              // Plugin
              handled = delegate->OnKeyDown(key);

          if (!handled)
              OnKeyDown(x, y, key);
      }
      break;
      case XCB_KEY_RELEASE:
      {
          xcb_key_release_event_t *ke = (xcb_key_release_event_t *)evt;

          float x = ke->event_x;
          float y = ke->event_y;

          xcb_keycode_t keycode = ke->detail;

          char utf8[7];
          uint32_t vk = ConvertKeyToVK(keycode, ke->state, utf8);
          
          IKeyPress key(utf8, vk);
          
          // Modifiers
          GetKeyModifiers(ke->state, &key.S, &key.C, &key.A);
          
          OnKeyUp(x, y, key);
      }
      break;
      
      default:
        break;
    }
  }
  mBaseWindowHandler(mPlugWnd, evt, mBaseWindowData);
}

void IGraphicsLinux::SetIntegration(void* mainLoop)
{
  xcbt_embed* e = static_cast<xcbt_embed*>(mainLoop);

  if (!e)
  {
    if (mEmbed)
    {
      if (mX)
      {
        // DBGMSG("asked to unset embedding, but X is still active\n"); that in fact how it goes, frame is unset before CloseWindow TODO: check why
        xcbt_embed_set(mX, nullptr);
      }
#if BL_FIX_CRASH_REOPEN
      if (mOwnEmbed)
        xcbt_embed_dtor(mEmbed);
#else
      xcbt_embed_dtor(mEmbed);
#endif
      mEmbed = nullptr;
    }
  }
  else
  {
    if (mEmbed)
      DBGMSG("BUG: embed is already set\n");
    else
      mEmbed = e;
  }
}

bool
IGraphicsLinux::GetScreenResolution(int *width, int *height)
{
    xcb_screen_t* si = xcbt_screen_info(mX, xcbt_default_screen(mX));
    if (si == NULL)
        return false;
    
    *width = si->width_in_pixels;
    *height = si->height_in_pixels;
    
    return true;
}

void* IGraphicsLinux::OpenWindow(void* pParent)
{
  xcbt_rect r = {0, 0, static_cast<int16_t>(WindowWidth()), static_cast<int16_t>(WindowHeight())};
  xcb_window_t xprt = (intptr_t) pParent;
  
#ifdef APP_API
  if (!mEmbed)
  {  
    SetIntegration(xcbt_embed_glib());

#if BL_FIX_CRASH_REOPEN
    mOwnEmbed = true;
#endif
  }
#endif

  if (!mEmbed)
  {
    DBGMSG("BUG: embed is not defined\n");
    return NULL;
  }

#ifdef IGRAPHICS_GL
  mX = xcbt_connect(XCBT_USE_GL|XCBT_INIT_ATOMS);
#else
  mX = xcbt_connect(0);
#endif
  if (!mX)
 {
    return NULL;
  }

#ifdef LV2_API
  if (!xprt)
  {
    // LV2 UI is created without parent by default, it may be found and even required with ui:parent feature, but the documentation
    // say that is not a good idea.
    xcb_screen_t* si = xcbt_screen_info(mX, xcbt_default_screen(mX));
    if (si)
    {
      xprt = si->root;
    }
  }
#endif

  // NOTE: In case plug-in report REAPER extension in REAPER, pParent is NOT XID (SWELL HWND? I have not checked yet)

#ifdef IGRAPHICS_GL
#ifdef IGRAPHICS_GL2
  mPlugWnd = xcbt_window_gl_create(mX, xprt, &r, 2, 1, 0);
#elif defined IGRAPHICS_GL3
  mPlugWnd = xcbt_window_gl_create(mX, xprt, &r, 3, 0, 0);
#else
  #error "Unsupported GL version"
#endif
#else

  mPlugWnd = xcbt_window_create(mX, xprt, &r);
#endif
  if (!mPlugWnd)
  {
    xcbt_disconnect(mX);
    mX = NULL;
    return NULL;
  }

  xcbt_window_set_handler(mPlugWnd, (xcbt_window_handler) WindowHandlerProxy, this, &mBaseWindowHandler, &mBaseWindowData);

  if (mEmbed && !xcbt_embed_set(mX, mEmbed))
  {
    DBGMSG("Could not embed into main event loop\n");
    xcbt_window_destroy(mPlugWnd);
    mPlugWnd = NULL;
    xcbt_disconnect(mX);
    mX = NULL;
    return NULL;
  }

  if (xcbt_window_draw_begin(mPlugWnd)) // GL context set
  { 
    OnViewInitialized(nullptr);
    SetScreenScale(1); // resizes draw context, calls DrawResize

    GetDelegate()->LayoutUI(this);
    SetAllControlsDirty();
    GetDelegate()->OnUIOpen();

    xcbt_window_draw_stop(mPlugWnd);
  }

  xcbt_timer_set(mX, IPLUG_TIMER_ID, 10, (xcbt_timer_cb) TimerHandlerProxy, this);

#ifdef APP_API
  xcbt_window_map(mPlugWnd);
  //xcbt_window_set_xembed_info(mPlugWnd);
#elif defined VST2_API
  xcbt_window_set_xembed_info(mPlugWnd);
#elif defined VST3_API
  xcbt_window_set_xembed_info(mPlugWnd);
#elif defined LV2_API
  xcbt_window_set_xembed_info(mPlugWnd);
#else
  #error "Map or not to map... that is the question"
#endif
  xcbt_sync(mX); // make sure everything is ready before reporting it is

  // Reset some state
  mCursorLock = false;
  mMouseVisible = true;

  return reinterpret_cast<void*>(xcbt_window_xwnd(mPlugWnd));
}

void IGraphicsLinux::CloseWindow()
{
  // #blulab
  mClosing = true;

  // Ensure the timer processing is finished
  // (this should not happen, this is just in case)
  while(mTimerProcessing)
    usleep(1000);

  if (mPlugWnd)
  {
    // #bluelab
    // Be sure we bind GL context before calling OnViewDestroyed()
    // because OnViewDestroyed() uses NanoVg calls to destroy NanoVg stuff
    // FIX: this fixes if we have 2 GL plugins in a DAW, we close one plugin window
    // and that made the second plugin display to freeze 
#ifdef IGRAPHICS_GL
    ActivateGLContext();
#endif
    
    OnViewDestroyed();

    // #bluelab
#ifdef IGRAPHICS_GL
    DeactivateGLContext();
#endif
    
    SetPlatformContext(nullptr);

    xcbt_window_destroy(mPlugWnd);
    mPlugWnd = NULL;
    xcbt_disconnect(mX);
    mX = NULL;
  }

  if (mEmbed)
  {
#if BL_FIX_CRASH_REOPEN
    if (mOwnEmbed)
      mEmbed->dtor(mEmbed);
#else
    mEmbed->dtor(mEmbed);
#endif
    
    mEmbed = nullptr;
  }

  mClosing = false;
}

void IGraphicsLinux::GetMouseLocation(float& x, float& y) const
{
  x = mCursorX;
  y = mCursorY;
}

void IGraphicsLinux::HideMouseCursor(bool hide, bool lock)
{
  if (mCursorHidden != hide)
  {
    mCursorHidden = hide;
    // https://stackoverflow.com/questions/57841785/how-to-hide-cursor-in-xcb

    if (mCursorHidden)
    {
      xcb_grab_pointer(xcbt_conn(mX), 1, mPlugWnd->wnd,
        XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, mPlugWnd->wnd, XCB_NONE, XCB_CURRENT_TIME);
      xcb_xfixes_hide_cursor_checked(xcbt_conn(mX), mPlugWnd->wnd);
    }
    else
    {
      xcb_ungrab_pointer(xcbt_conn(mX), XCB_CURRENT_TIME);
      xcb_xfixes_show_cursor_checked(xcbt_conn(mX), mPlugWnd->wnd);
    }
  }

  if (mCursorLock != lock)
  {
    mCursorLock = lock;

    if (mCursorLock)
      mMouseLockPos = IVec2(mCursorX, mCursorY);
    else
      mMouseLockPos = IVec2(0, 0);
  }
}

void IGraphicsLinux::MoveMouseCursor(float x, float y)
{
  xcbt_move_cursor(mPlugWnd, XCBT_WINDOW, (int)x, (int)y);
  mCursorX = mMouseLockPos.x;
  mCursorY = mMouseLockPos.y;
}

EMsgBoxResult IGraphicsLinux::ShowMessageBox(const char* text, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler)
{
  WDL_String command;
  WDL_String argument;

  auto pushArgument = [&]() {
    command.Append(&argument);
    command.Append(" ", 2);
  };

  argument.Set("zenity");
  pushArgument();

  argument.Set("--modal");
  pushArgument();

  argument.SetFormatted(strlen(caption) + 10, "\"--title=%s\"", caption);
  pushArgument();

  argument.SetFormatted(strlen(text) + 10, "\"--text=%s\"", text);
  pushArgument();

  switch (type)
  {
    case EMsgBoxType::kMB_OK:
      argument.Set("--info");
      pushArgument();
      break;
    case EMsgBoxType::kMB_OKCANCEL:
      argument.SetFormatted(64, "--ok-label=Ok");
      pushArgument();
      argument.SetFormatted(64, "--cancel-label=Cancel");
      pushArgument();
      argument.Set("--question");
      pushArgument();
      break;
    case EMsgBoxType::kMB_RETRYCANCEL:
      argument.SetFormatted(64, "--ok-label=Retry");
      pushArgument();
      argument.SetFormatted(64, "--cancel-label=Cancel");
      pushArgument();
      argument.Set("--question");
      pushArgument();
      break;
    case EMsgBoxType::kMB_YESNO:
      argument.Set("--question");
      pushArgument();
      break;
    case EMsgBoxType::kMB_YESNOCANCEL:
      argument.Set("--question");
      pushArgument();
      argument.Set("--extra-button=Cancel");
      pushArgument();
      break;
  }

  EMsgBoxResult result = kNoResult;
  WDL_String sStdout;
  WDL_String sStdin;
  int status;

  if (RunSubprocess(command.Get(), sStdout, sStdin, &status) != 0)
  {
    if (completionHandler)
      completionHandler(result);

    return result;
  }

  switch (type)
  {
    case EMsgBoxType::kMB_OK:
      result = kOK;
      break;
    case EMsgBoxType::kMB_OKCANCEL:
      switch (status)
      {
        case 0:
          result = kOK;
          break;
        default:
          result = kCANCEL;
          break;
      }
      break;
    case EMsgBoxType::kMB_RETRYCANCEL:
      switch (status)
      {
        case 0:
          result = kRETRY;
          break;
        default:
          result = kCANCEL;
          break;
      }
      break;
    case EMsgBoxType::kMB_YESNO:
      switch (status)
      {
        case 0:
          result = kYES;
          break;
        default:
          result = kNO;
          break;
      }
      break;
    case EMsgBoxType::kMB_YESNOCANCEL:
      switch (status)
      {
        case 0:
          result = kYES;
          break;
        case 1:
          // zenity output our extra button text
          if (sStdout.GetLength() > 0)
            result = kCANCEL;
          else
            result = kNO;
          break;
        default:
          result = kCANCEL;
          break;
      }
      break;
  }

  if (completionHandler)
    completionHandler(result);

  return result;
}

bool IGraphicsLinux::RevealPathInExplorerOrFinder(WDL_String& path, bool select)
{
  WDL_String args;
  args.SetFormatted(path.GetLength() + 40, "xdg-open \"%s\" ", path.Get());

  WDL_String sOut, sIn;
  int status;

  if (RunSubprocess(args.Get(), sOut, sIn, &status) != 0)
  {
    return false;
  }

  return true;
}

void IGraphicsLinux::PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* extensions)
{
  if (!WindowIsOpen())
  {
    fileName.Set("");
    return;
  }

  WDL_String tmp;
  WDL_String args;

  //#bluelab
  // No need to cd , The "--filename" argument will open zenity on the right folder.
  // And if we don't cd, it will be possible to do a "save as",
  // with the filename already pre-filled!
#if 0
  args.AppendFormatted(path.GetLength() + 10, "cd \"%s\"; ", path.Get());
#endif

  // #bluelab
  // Instead, cd to home directory
#if 1
  args.Append("cd ;");
#endif
  
  args.Append("zenity --file-selection ");

  if (action == EFileAction::Save)
  {
    args.Append("--save --confirm-overwrite ");
  }
  if (fileName.GetLength() > 0)
  {
    args.AppendFormatted(fileName.GetLength() + 20, "\"--filename=%s\" ", fileName.Get());
  }

  // NOTE: maybe there is a bug here, infinite loop due to "," note removed
  // after processing
#if !BL_FILE_FILTER_PATCH
  if (extensions)
  {
    // Split the string at commas and then append each format specifier
    const char* ext = extensions;
    while (*ext)
    {
      const char* start = ext;
      while (*ext && *ext != ',')
        ext++;
      tmp.Set(start, ext - start);
      args.AppendFormatted(256, "\"--file-filter=%s | %s\" ", tmp.Get(), tmp.Get());
      if (!(*ext))
        ext++;
    }
  }
#else
  if (extensions)
  {
      // Warning: Zenity seems to not manage case for extensions
      args.AppendFormatted(256, "--file-filter=\"%s\" ", extensions);
  }
#endif
  
  WDL_String sStdout;
  WDL_String sStdin;
  int status;
  
  if (RunSubprocess(args.Get(), sStdout, sStdin, &status) != 0)
  {
    fileName.Set("");
    return;
  }

  if (status == 0)
  {
    fileName.Set(sStdout.Get());

#if BL_FILE_FILTER_PATCH
    // Suppress the '\n' at the end of the filename
    // (later this won't open the file otherwise)
    if (fileName.GetLength() >= 1)
        fileName.SetLen(fileName.GetLength() - 1);
#endif
  }
  else
  {
    fileName.Set("");
  }
}

void IGraphicsLinux::PromptForDirectory(WDL_String& dir)
{
  if (!WindowIsOpen())
  {
    dir.Set("");
    return;
  }

  WDL_String args;
  args.Append("zenity --file-selection --directory ");
  if (dir.GetLength() > 0)
  {
    args.AppendFormatted(dir.GetLength() + 20, "\"--filename=%s\"", dir.Get());
  }
  
  WDL_String sStdout;
  WDL_String sStdin;
  int status;
  if (RunSubprocess(args.Get(), sStdout, sStdin, &status) != 0)
  {
    dir.Set("");
    return;
  }

  if (status == 0)
  {
    dir.Set(sStdout.Get()); 
  }
  else
  {
    dir.Set("");
  }
}

bool IGraphicsLinux::PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func)
{
  WDL_String args;
  args.Append("zenity --color-selection ");

  if (str)
    args.AppendFormatted(strlen(str) + 20, "\"--title=%s\" ", str);
  
  args.AppendFormatted(100, "\"--color=rgba(%d,%d,%d,%f)\" ", color.R, color.G, color.B, (float)color.A / 255.f);

  bool ok = false;

  WDL_String sOut;
  WDL_String sIn;
  int status;

  if (RunSubprocess(args.Get(), sOut, sIn, &status) != 0)
  {
    return false;
  }
  else
  {
    if (strncmp(sOut.Get(), "rgba", 4) == 0)
    {
      // Parse RGBA
      int cr, cg, cb;
      float ca;
      int ct = sscanf(sOut.Get(), "rgba(%d,%d,%d,%f)", &cr, &cg, &cb, &ca);
      if (ct == 4)
      {
        color = IColor((int)(255.f * ca), cr, cg, cb);
        ok = true;
      }
    }
    else if (strncmp(sOut.Get(), "rgb", 3) == 0)
    {
      // Parse RGB
      int cr, cg, cb;
      int ct = sscanf(sOut.Get(), "rgb(%d,%d,%d)", &color.R, &color.G, &color.B);
      if (ct == 3)
      {
        color = IColor(255, cr, cg, cb);
        ok = true;
      }
    }

    if (ok && func)
      func(color);

    return ok;
  }
}

bool IGraphicsLinux::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  WDL_String args;
  args.SetFormatted(strlen(url) + 40, "xdg-open \"%s\" ", url);

  WDL_String sOut, sIn;
  int status;

  if (RunSubprocess(args.Get(), sOut, sIn, &status) != 0)
  {
    return false;
  }

  return true;
}

bool IGraphicsLinux::GetTextFromClipboard(WDL_String& str)
{
  int length = 0;
  const char* data = xcbt_clipboard_get_utf8(mPlugWnd, &length);

  if (data)
  {
    str.Set(data, length);
    return true;
  }
  else
  {
    return false;
  }
}

bool IGraphicsLinux::SetTextInClipboard(const char* str)
{
  return xcbt_clipboard_set_utf8(mPlugWnd, str) == 1;
}

void IGraphicsLinux::PlatformResize(bool parentHasResized)
{
  if (WindowIsOpen())
  {
    xcb_connection_t *conn = xcbt_conn(mX);
    xcb_window_t w = xcbt_window_xwnd(mPlugWnd);
    uint32_t values[] = { static_cast<uint32_t>(WindowWidth() * GetScreenScale()), static_cast<uint32_t>(WindowHeight() * GetScreenScale()) };
    xcb_configure_window(conn, w, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
    //DBGMSG("INFO: resized to %ux%u\n", values[0], values[1]);
    if (!parentHasResized)
    {
        //DBGMSG("WARNING: parent is not resized, but I (should) have no control on it on X... XEMBED?\n");
      xcb_window_t prt = xcbt_window_xprt(mPlugWnd);
      if (prt)
      {
        xcb_configure_window(conn, prt, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
      }
    }
    xcbt_flush(mX);
  }
}

void IGraphicsLinux::RequestFocus()
{
  xcb_set_input_focus_checked(xcbt_conn(mX), XCB_INPUT_FOCUS_POINTER_ROOT, mPlugWnd->wnd, XCB_CURRENT_TIME);
}

//TODO: move these
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

IFontDataPtr IGraphicsLinux::Font::GetFontData()
{
  if (mFontData.GetSize() == 0)
  {
    IFontDataPtr pData;
    int file = open(mFileName.Get(), O_RDONLY);
    if (file >= 0)
    {
      struct stat sb;
      if (fstat(file, &sb) == 0)
      {
        int fontSize = static_cast<int>(sb.st_size);
        void* pFontMem = mmap(NULL, fontSize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, file, 0);
        if (pFontMem != MAP_FAILED)
        {
          pData = std::make_unique<IFontData>(pFontMem, fontSize, 0);
          munmap(pFontMem, fontSize);
        }
      }
      close(file);
    }
    return pData;
  }
  else
  {
    return std::make_unique<IFontData>((const void*)mFontData.Get(), mFontData.GetSize(), 0);
  }
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
  WDL_String fullPath;
  const EResourceLocation fontLocation = LocateResource(fileNameOrResID, "ttf", fullPath, GetBundleID(), GetWinModuleHandle(), nullptr);

  if ((fontLocation == kNotFound) || (fontLocation != kAbsolutePath) )
  {
    return nullptr;
  }

  return PlatformFontPtr(new Font(fullPath));
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, void* pData, int dataSize)
{
  WDL_String name;
  name.Set(fontID);
  return PlatformFontPtr(new Font(name, pData, dataSize));
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
  WDL_String fullPath;
  const char* styleString;

  switch (style)
  {
    case ETextStyle::Bold: styleString = "bold"; break;
    case ETextStyle::Italic: styleString = "italic"; break;
    default: styleString = "regular";
  }

  FcConfig* config = FcInitLoadConfigAndFonts(); // TODO: init/fini for plug-in lifetime
  FcPattern* pat = FcPatternBuild(nullptr, FC_FAMILY, FcTypeString, fontName, FC_STYLE, FcTypeString, styleString, nullptr);
  FcConfigSubstitute(config, pat, FcMatchPattern);
  FcResult result;
  FcPattern* font = FcFontMatch(config, pat, &result);

  if (font)
  {
    FcChar8* file;
    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
    {
      fullPath.Set((const char*) file);
    }
    FcPatternDestroy(font);
  }

  FcPatternDestroy(pat);
  FcConfigDestroy(config);

  return PlatformFontPtr(fullPath.Get()[0] ? new Font(fullPath) : nullptr);
}

uint32_t IGraphicsLinux::GetUserDblClickTimeout()
{
  // Default to 400
  uint32_t timeout = 400;

  // Source: forum.kde.org/viewtopic.php?f=289&t=153755
  // Read $HOME/.gtkrc-2.0; Var: gtk-double-click-time
  // Read $HOME/.config/gtk-3.0/settings.ini ; Var: gtk-double-click-time
  // Read $HOME/.config/kdeglobals, [KDE] section, Var: DoubleClickInterval
  return timeout;
}

IGraphicsLinux::IGraphicsLinux(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
  : IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
  // #bluelab
  mClosing = false;
  mTimerProcessing = false;
  mPrevTimeStampPaint = 0.0;
  mTimerDelay = (int)(1000.0/fps);
}

IGraphicsLinux::~IGraphicsLinux()
{
  CloseWindow();

#if BL_FIX_CRASH_REOPEN
  if (mOwnEmbed)
    xcbt_embed_dtor(mEmbed);
#else
  xcbt_embed_dtor(mEmbed);
#endif
}

// #bluelab
void
IGraphicsLinux::ActivateGLContext()
{
  xcbt_window_draw_begin(mPlugWnd);
}

void
IGraphicsLinux::DeactivateGLContext()
{
  xcbt_window_draw_end(mPlugWnd);
}

// #bluelab
void
IGraphicsLinux::GetKeyModifiers(uint32_t mask, bool *S, bool *C, bool *A)
{
    *S = false;
    *C = false;
    *A = false;

    if (mask & XCB_KEY_BUT_MASK_SHIFT)
        *S = true;
    
    if (mask & XCB_KEY_BUT_MASK_CONTROL)
        *C = true;

    if (mask & XCB_KEY_BUT_MASK_MOD_1)
        *C = true;
}

// #bluelab
//xkb key to VK conversion
uint32_t
IGraphicsLinux::ConvertKeyToVK(uint32_t keycode,
                               uint16_t modifier,
                               char utf8[7])
{
    uint32_t vk;
    memset(utf8, 0, 7);

    vk = ConvertSpecialKeyToVK(keycode, modifier);
    if (vk != 0)
        return vk;

    vk = ConvertSimpleKeyToVK(keycode, modifier, utf8);

    return vk;
}

uint32_t
IGraphicsLinux::ConvertSpecialKeyToVK(uint32_t keycode, uint16_t modifier)
{
  xkb_keysym_t keysym = xcbt_keyboard_get_keysym(mX, keycode, modifier);

#if 0 // DEBUG
    // Keysym name
    char keysym_name[64];
    xcbt_keyboard_get_keysym_name(keysym, keysym_name);
    fprintf(stderr, "keycode: %d keysym %d name: %s\n",
            keycode, keysym, keysym_name);
#endif
    
  switch(keysym)
  {
    case XK_Home: return kVK_HOME;
    case XK_End: return kVK_END;
    case XK_Up: return kVK_UP;
    case XK_Down: return kVK_DOWN;
    case XK_Left: return kVK_LEFT;
    case XK_Right: return kVK_RIGHT;
    case XK_Page_Up: return kVK_PRIOR;
    case XK_Page_Down: return kVK_NEXT;
    case XK_Insert: return kVK_INSERT;
    case XK_Delete: return kVK_DELETE;
    case XK_Escape: return kVK_ESCAPE;
    case XK_BackSpace: return kVK_BACK;
    case XK_Return: return kVK_RETURN;
    case XK_Tab: return kVK_TAB;
    case XK_F1: return kVK_F1;
    case XK_F2: return kVK_F2;
    case XK_F3: return kVK_F3;
    case XK_F4: return kVK_F4;
    case XK_F5: return kVK_F5;
    case XK_F6: return kVK_F6;
    case XK_F7: return kVK_F7;
    case XK_F8: return kVK_F8;
    case XK_F9: return kVK_F9;
    case XK_F10: return kVK_F10;
    case XK_F11: return kVK_F11;
    case XK_F12: return kVK_F12;
    case XK_KP_0: return kVK_NUMPAD0;
    case XK_KP_1: return kVK_NUMPAD1;
    case XK_KP_2: return kVK_NUMPAD2;
    case XK_KP_3: return kVK_NUMPAD3;
    case XK_KP_4: return kVK_NUMPAD4;
    case XK_KP_5: return kVK_NUMPAD5;
    case XK_KP_6: return kVK_NUMPAD6;
    case XK_KP_7: return kVK_NUMPAD7;
    case XK_KP_8: return kVK_NUMPAD8;
    case XK_KP_9: return kVK_NUMPAD9;
    case XK_KP_Multiply: return kVK_MULTIPLY;
    case XK_KP_Add: return kVK_ADD;
    case XK_KP_Separator: return kVK_SEPARATOR;
    case XK_KP_Subtract: return kVK_SUBTRACT;
    case XK_KP_Decimal: return kVK_DECIMAL;
    case XK_KP_Divide: return kVK_DIVIDE;
    case XK_Num_Lock: return kVK_NUMLOCK;

    // Modifiers
    case XK_Shift_L: return kVK_SHIFT;
    case XK_Shift_R: return kVK_SHIFT;
    case XK_Control_L: return kVK_CONTROL;
    case XK_Control_R: return kVK_CONTROL;
    case XK_Caps_Lock: return kVK_CAPITAL;
    case XK_Shift_Lock: return kVK_CAPITAL; // ?
    //case XK_Meta_L:
    //case XK_Meta_R:
    //case XK_Alt_L: return kVK_ALT;
    //case XK_Alt_R: return kVK_ALT;
    case XK_Super_L: return kVK_LWIN;
    case XK_Super_R: return kVK_LWIN;
    case XK_Hyper_L: return kVK_LWIN;
    case XK_Hyper_R: return kVK_LWIN;
  }
  return 0;
}

uint32_t
IGraphicsLinux::ConvertSimpleKeyToVK(uint32_t keycode,
                                     uint16_t modifier,
                                     char utf8[5])
{
    xkb_keysym_t keysym = xcbt_keyboard_get_keysym(mX, keycode, modifier);

    xcbt_keyboard_get_keysym_utf8(keysym, utf8);

#if 0 // DEBUG
    // Keysym name
    char keysym_name[64];
    xcbt_keyboard_get_keysym_name(keysym, keysym_name);
    fprintf(stderr, "keycode: %d keysym %d utf8: %s name: %s\n",
            keycode, keysym, utf8, keysym_name);
#endif

    if ((keysym >= XK_a) && (keysym <= XK_z))
    {
        uint32_t vk = kVK_A + (keysym - XK_a);
        return vk;
    }
    
    if ((keysym >= XK_A) && (keysym <= XK_Z))
    {
        uint32_t vk = kVK_A + (keysym - XK_A);
        return vk;
    }

    if ((keysym >= XK_0) && (keysym <= XK_9))
    {
        uint32_t vk = kVK_0 + (keysym - XK_0);
        return vk;
    }

    switch(keysym)
    {
        case XK_space: return kVK_SPACE;
        case XK_plus: return kVK_ADD;
        case XK_minus: return kVK_SUBTRACT;
        case XK_period: return kVK_DECIMAL;
        case XK_comma: return kVK_COMMA;
        // Add your own here if you need other keys...
    }

    return 0;
}

#ifndef NO_IGRAPHICS
  #if defined IGRAPHICS_SKIA
    #include "IGraphicsSkia.cpp"
  #elif defined IGRAPHICS_NANOVG
    #include "IGraphicsNanoVG.cpp"
    #ifdef IGRAPHICS_FREETYPE
      #define FONS_USE_FREETYPE
    #endif
      #include "nanovg.c"
  #else
    #error
  #endif
#endif
