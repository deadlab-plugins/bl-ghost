/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

/**
 * @file
 * @brief IPlugAPIBase implementation
 */

#include <cmath>
#include <cstdio>
#include <ctime>
#include <cassert>

#include "IPlugAPIBase.h"

// Ableton11, Win10, VST2: mutex deadlock when resizing GUI from host UI
#if (defined WIN32) && (defined VST2_API)
#define FIX_ABLETON11_WIN10_FREEZE 1
#else
#define FIX_ABLETON11_WIN10_FREEZE 0
#endif

using namespace iplug;

IPlugAPIBase::IPlugAPIBase(Config c, EAPI plugAPI)
  : IPluginBase(c.nParams, c.nPresets)
{
  mUniqueID = c.uniqueID;
  mMfrID = c.mfrID;
  mVersion = c.vendorVersion;
  mPluginName.Set(c.pluginName, MAX_PLUGIN_NAME_LEN);
  mProductName.Set(c.productName, MAX_PLUGIN_NAME_LEN);
  mMfrName.Set(c.mfrName, MAX_PLUGIN_NAME_LEN);
  mHasUI = c.plugHasUI;
  mHostResize = c.plugHostResize;
  SetEditorSize(c.plugWidth, c.plugHeight);
  SetSizeConstraints(c.plugMinWidth, c.plugMaxWidth, c.plugMinHeight, c.plugMaxHeight);
  mStateChunks = c.plugDoesChunks;
  mAPI = plugAPI;
  mBundleID.Set(c.bundleID);

  Trace(TRACELOC, "%s:%s", c.pluginName, CurrentTime());
  
  mParamDisplayStr.Set("", MAX_PARAM_DISPLAY_LEN);

  // #bluelab
  mIsTimerDisabled = false;
}

IPlugAPIBase::~IPlugAPIBase()
{
  if(mTimer)
  {
    mTimer->Stop();
  }

  TRACE
}

void IPlugAPIBase::OnHostRequestingImportantParameters(int count, WDL_TypedBuf<int>& results)
{
  if(NParams() > count)
  {
    for (int i = 0; i < count; i++)
      results.Add(i);
  }
}

void IPlugAPIBase::CreateTimer()
{
#if defined(OS_LINUX) && !defined(APP_API) && !defined(VST3_API)
  auto cb = [this](Timer& timer) { if (!HasUI()) { OnTimer(*mTimer); } };
  mTimer = std::unique_ptr<Timer>(Timer::Create(cb, IDLE_TIMER_RATE));
#elif defined(OS_LINUX) && defined(VST3_API)
  // Do nothing.
#else
  mTimer = std::unique_ptr<Timer>(Timer::Create(std::bind(&IPlugAPIBase::OnTimer, this, std::placeholders::_1), IDLE_TIMER_RATE));
#endif
}

bool IPlugAPIBase::CompareState(const uint8_t* pIncomingState, int startPos) const
{
  bool isEqual = true;
  
  const double* data = (const double*) pIncomingState + startPos;
  
  // dirty hack here because protools treats param values as 32 bit int and in IPlug they are 64bit float
  // if we memcmp() the incoming state with the current they may have tiny differences due to the quantization
  for (int i = 0; i < NParams(); i++)
  {
    float v = (float) GetParam(i)->Value();
    float vi = (float) *(data++);
    
    isEqual &= (std::fabs(v - vi) < 0.00001);
  }
  
  return isEqual;
}

bool IPlugAPIBase::EditorResizeFromUI(int viewWidth, int viewHeight, bool needsPlatformResize)
{  
  if (needsPlatformResize)
    return EditorResize(viewWidth, viewHeight);
  else
    return true;
}

#pragma mark -

void IPlugAPIBase::SetHost(const char* host, int version)
{
  assert(mHost == kHostUninit);
    
  mHost = LookUpHost(host);
  mHostVersion = version;
  
  WDL_String vStr;
  GetVersionStr(version, vStr);
  Trace(TRACELOC, "host_%sknown:%s:%s", (mHost == kHostUnknown ? "un" : ""), host, vStr.Get());
    
  HostSpecificInit();
  OnHostIdentified();
}

void IPlugAPIBase::SetParameterValue(int idx, double normalizedValue)
{
  Trace(TRACELOC, "%d:%f", idx, normalizedValue);
  GetParam(idx)->SetNormalized(normalizedValue);
  InformHostOfParamChange(idx, normalizedValue);
  OnParamChange(idx, kUI);
}

void IPlugAPIBase::DirtyParametersFromUI()
{
  for (int p = 0; p < NParams(); p++)
  {
    double normalizedValue = GetParam(p)->GetNormalized();
    InformHostOfParamChange(p, normalizedValue);
  }
}

void IPlugAPIBase::SendParameterValueFromAPI(int paramIdx, double value, bool normalized)
{
  if (normalized)
    value = GetParam(paramIdx)->FromNormalized(value);
  
  mParamChangeFromProcessor.Push(ParamTuple { paramIdx, value } );
}

// #bluelab
void
IPlugAPIBase::SetTimerDisabled(bool flag)
{
  // Using mutex here prevents from additional Ableton11/Win10/resize GUI crash
  ENTER_PARAMS_MUTEX;

  mIsTimerDisabled = flag;

  LEAVE_PARAMS_MUTEX;
}

void IPlugAPIBase::OnTimer(Timer& t)
{
    // #bluelab
#if FIX_ABLETON11_WIN10_FREEZE
  // On Ableton11/Win10/VST2, we would have mutex dead locks if resizing GUI
  // from host UI, if ever we continue to process OnTimer() while resizing
  if (mIsTimerDisabled)
    return;
#endif

#if FIX_ABLETON11_WIN10_FREEZE
  ENTER_PARAMS_MUTEX;
#endif
  
  if(HasUI())
  {
// VST3 ********************************************************************************
#if defined VST3P_API || defined VST3_API
    while (mMidiMsgsFromProcessor.ElementsAvailable())
    {
      IMidiMsg msg;
      mMidiMsgsFromProcessor.Pop(msg);
#ifdef VST3P_API // distributed
      TransmitMidiMsgFromProcessor(msg);
#else
      SendMidiMsgFromDelegate(msg);
#endif
    }

    while (mSysExDataFromProcessor.ElementsAvailable())
    {
      SysExData msg;
      mSysExDataFromProcessor.Pop(msg);
#ifdef VST3P_API // distributed
      TransmitSysExDataFromProcessor(msg);
#else
      SendSysexMsgFromDelegate({msg.mOffset, msg.mData, msg.mSize});
#endif
    }
// !VST3 ******************************************************************************
#else
    // #bluelab: added mutex. We change parameters from timer!
    // If at the same time parameter is changed from ProcessBlock(), this is bad
    // (Added for AutoGain)
#if !FIX_ABLETON11_WIN10_FREEZE
    ENTER_PARAMS_MUTEX;
#endif
    
    while(mParamChangeFromProcessor.ElementsAvailable())
    {
      ParamTuple p;
      mParamChangeFromProcessor.Pop(p);
      SendParameterValueFromDelegate(p.idx, p.value, false);
    }

#if !FIX_ABLETON11_WIN10_FREEZE
    LEAVE_PARAMS_MUTEX;
#endif
    
    while (mMidiMsgsFromProcessor.ElementsAvailable())
    {
      IMidiMsg msg;
      mMidiMsgsFromProcessor.Pop(msg);
      SendMidiMsgFromDelegate(msg);
    }
    
    while (mSysExDataFromProcessor.ElementsAvailable())
    {
      SysExData msg;
      mSysExDataFromProcessor.Pop(msg);
      SendSysexMsgFromDelegate({msg.mOffset, msg.mData, msg.mSize});
    }
#endif
  }
  
  // On VST2 Linux we call OnIdle from the VST2 API
#if !(defined(OS_LINUX) && defined(VST2_API))
  OnIdle();
#endif

#if FIX_ABLETON11_WIN10_FREEZE
  LEAVE_PARAMS_MUTEX;
#endif
}

void IPlugAPIBase::SendMidiMsgFromUI(const IMidiMsg& msg)
{
  DeferMidiMsg(msg); // queue the message so that it will be handled by the processor
  EDITOR_DELEGATE_CLASS::SendMidiMsgFromUI(msg); // for remote editors
}

void IPlugAPIBase::SendSysexMsgFromUI(const ISysEx& msg)
{
  DeferSysexMsg(msg); // queue the message so that it will be handled by the processor
  EDITOR_DELEGATE_CLASS::SendSysexMsgFromUI(msg); // for remote editors
}

void IPlugAPIBase::SendArbitraryMsgFromUI(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  OnMessage(msgTag, ctrlTag, dataSize, pData); // IPlugAPIBase implementation handles non distributed plug-ins - just call OnMessage() directly
  
  EDITOR_DELEGATE_CLASS::SendArbitraryMsgFromUI(msgTag, ctrlTag, dataSize, pData);
}