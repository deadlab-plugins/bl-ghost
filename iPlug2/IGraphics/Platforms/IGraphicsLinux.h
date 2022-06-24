/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IGraphics_select.h"
#include <memory>
#include <xcbt.h>

// #bluelab
// FIX: vst2/vst3: open in Reaper, display the plugin GUI,
// close the plug window then re-open it => Reaper crashes
#define BL_FIX_CRASH_REOPEN 1

BEGIN_IPLUG_NAMESPACE
class Timer;

BEGIN_IGRAPHICS_NAMESPACE

/** IGraphics platform class for linux
*   @ingroup PlatformClasses
*/
class IGraphicsLinux final : public IGRAPHICS_DRAW_CLASS
{
  class Font;
public:
  IGraphicsLinux(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  virtual ~IGraphicsLinux();

  void* OpenWindow(void* pWindow) override;
  void CloseWindow() override;
  void* GetWindow() override { return (void *)(intptr_t)mPlugWnd; }
  bool WindowIsOpen() override { return (mPlugWnd); }
  void PlatformResize(bool parentHasResized) override;
  void GetMouseLocation(float& x, float& y) const override;
  void HideMouseCursor(bool hide, bool lock) override;
  void MoveMouseCursor(float x, float y) override;
  EMsgBoxResult ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler) override;
  void ForceEndUserEdit() override { /* NO-OP */ }
  void DrawResize() override;
  const char* GetPlatformAPIStr() override { return "Linux"; }
  void UpdateTooltips() override { /* NO-OP */ }; 
  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override;
  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* extensions) override;
  void PromptForDirectory(WDL_String& dir) override;
  bool PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func) override;
  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override;
  static int GetUserOSVersion();
  bool GetTextFromClipboard(WDL_String& str) override;
  bool SetTextInClipboard(const char* str) override;

  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, void* pData, int dataSize) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style) override;

  void CachePlatformFont(const char* fontID, const PlatformFontPtr& font) override { } // No reason to cache (no universal font handle)
  void SetIntegration(void* mainLoop) override;

  // #bluelab
  bool GetScreenResolution(int *width, int *height) override;
  
protected:
  IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, bool& isAsync) override { /* NO-OP */ return nullptr; }
  void CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str) override { /* NO-OP */ } 
  void RequestFocus();

  friend class IGraphics;
private:
  // #bluelab
#ifdef IGRAPHICS_GL
  void ActivateGLContext() override;
  void DeactivateGLContext() override;
#endif
  
  xcbt mX = NULL;
  xcbt_embed* mEmbed = NULL;
#if BL_FIX_CRASH_REOPEN
  bool mOwnEmbed = false;
#endif
  xcbt_window mPlugWnd = NULL;
  xcbt_window_handler mBaseWindowHandler;
  void* mBaseWindowData;

  /** Double-click timeout in milliseconds */
  uint32_t mDblClickTimeout = 400;
  xcb_timestamp_t mLastLeftClickStamp; // it will be not zero in case there is a chance for double click

  IVec2 mMouseLockPos;
  bool mMouseVisible;

  void Paint(IRECTList &rects);
  inline IMouseInfo GetMouseInfo(int16_t x, int16_t y, int16_t state);
  inline IMouseInfo GetMouseInfoDeltas(float& dX, float& dY, int16_t x, int16_t y, int16_t state);
  void WindowHandler(xcb_generic_event_t* evt);
  void TimerHandler(int id);

  static uint32_t GetUserDblClickTimeout();
  static void WindowHandlerProxy(xcbt_window xw, xcb_generic_event_t* evt, IGraphicsLinux* pGraphics) { pGraphics->WindowHandler(evt); }
  static void TimerHandlerProxy(xcbt x, int timer_id, IGraphicsLinux* pGraphics) { pGraphics->TimerHandler(timer_id); }

  // #bluelab
  static void GetKeyModifiers(uint32_t mask, bool *S, bool *C, bool *A);

  uint32_t ConvertKeyToVK(uint32_t keycode, uint16_t modifier, char utf8[7]);
  uint32_t ConvertSpecialKeyToVK(uint32_t keycode, uint16_t modifier);
  uint32_t ConvertSimpleKeyToVK(uint32_t keycode, uint16_t modifier, char utf8[7]);

  // #bluelab
  // Avoid conflicts between timer and window close at the same time
  bool mClosing;
  bool mTimerProcessing;

  double mPrevTimeStampPaint = 0.;
  int mTimerDelay;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
