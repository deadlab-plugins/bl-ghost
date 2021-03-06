/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IPlugFaust
 */

#include <memory>

#define FAUSTCLASS_POLY mydsp_poly

#define FAUST_UI_INTERVAL 100 //ms

#include "faust/dsp/poly-dsp.h"
#include "faust/gui/UI.h"
#include "faust/gui/MidiUI.h"
#include "faust/midi/iplug2-midi.h"
#include "assocarray.h"

#include "IPlugAPIBase.h"

#include "Oversampler.h"

#ifndef FAUST_SHARE_PATH
  #if defined OS_MAC || defined OS_LINUX
    #define FAUST_SHARE_PATH "/usr/local/share/faust/"
  #else
   #define FAUST_SHARE_PATH "C:\\Program Files\\Faust\\share\\faust"
  #endif
#endif

BEGIN_IPLUG_NAMESPACE
using MidiHandlerPtr = std::unique_ptr<iplug2_midi_handler>;

/** This abstract interface is used by the IPlug FAUST architecture file and the IPlug libfaust JIT compiling class FaustGen
 * In order to provide a consistent interface to FAUST DSP whether using the JIT compiler or a compiled C++ class */
class IPlugFaust : public UI, public Meta
{
public:
  
  IPlugFaust(const char* name, int nVoices = 1, int rate = 1);

  virtual ~IPlugFaust()
  {
    mParams.Empty(true);
  }

  IPlugFaust(const IPlugFaust&) = delete;
  IPlugFaust& operator=(const IPlugFaust&) = delete;
    
  virtual void Init() = 0;

  // NO-OP in the base class
  virtual void SetMaxChannelCount(int maxNInputs, int maxNOutputs) {}
  
  /** In FaustGen this is implemented, so that the SVG files generated by a specific instance can be located. The path to the SVG file for process.svg will be returned.
   * There is a NO-OP implementation here so that when not using the JIT compiler, the same class can be used interchangeably
   * @param path The absolute path to process.svg for this instance. */
  virtual void GetDrawPath(WDL_String& path) {}

  /** Call this method from FaustGen in order to execute a shell command and compile the C++ code against the IPlugFaust_arch architecture file
   * There is a NO-OP implementation here so that when not using the JIT compiler, the same class can be used interchangeably
   * @return \c true on success */
  static bool CompileCPP() { return true; }

  static void SetAutoRecompile(bool enable) {}
  
  void FreeDSP()
  {
    mMidiHandler->stopMidi();
    mMidiUI = nullptr;
    mDSP = nullptr;
    mMidiHandler = nullptr;
  }
  
  void SetOverSamplingRate(int rate)
  {
    if(mOverSampler)
      mOverSampler->SetOverSampling(OverSampler<sample>::RateToFactor(rate));
  }

  // Unique methods
  void SetSampleRate(double sampleRate)
  {
    int multiplier = 1;
    
    if(mOverSampler)
      multiplier = mOverSampler->GetRate();
    
    if (mDSP) {
      mDSP->init(((int) sampleRate) * multiplier);
      SyncFaustParams();
    }
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    mMidiHandler->decodeMessage(msg);
  }

  virtual void ProcessBlock(sample** inputs, sample** outputs, int nFrames);

  void SetParameterValueNormalised(int paramIdx, double normalizedValue);
  
  void SetParameterValue(int paramIdx, double nonNormalizedValue);

  void SetParameterValue(const char* labelToLookup, double nonNormalizedValue);

  int CreateIPlugParameters(IPlugAPIBase* pPlug, int startIdx = 0, int endIdx = -1, bool setToDefault = true);

  int NParams() const
  {
    return mParams.GetSize();
  }
  
  void SyncFaustParams();

  // Meta
  void declare(const char *key, const char *value) override
  {
    // TODO:
  }

  // UI

  // TODO:
  void openTabBox(const char *label) override {}
  void openHorizontalBox(const char *label) override {}
  void openVerticalBox(const char *label) override {}
  void closeBox() override {}
  
  void addButton(const char *label, FAUSTFLOAT *zone) override
  {
    AddOrUpdateParam(IParam::kTypeBool, label, zone);
  }

  void addCheckButton(const char *label, FAUSTFLOAT *zone) override
  {
    AddOrUpdateParam(IParam::kTypeBool, label, zone);
  }

  void addVerticalSlider(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) override
  {
    AddOrUpdateParam(IParam::kTypeDouble, label, zone, init, min, max, step);
  }

  void addHorizontalSlider(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) override
  {
    AddOrUpdateParam(IParam::kTypeDouble, label, zone, init, min, max, step);
  }

  void addNumEntry(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) override
  {
    AddOrUpdateParam(IParam::kTypeEnum, label, zone, init, min, max, step);
  }

  // TODO:
  void addHorizontalBargraph(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT min, FAUSTFLOAT max) override {}
  void addVerticalBargraph(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT min, FAUSTFLOAT max) override {}
  void addSoundfile(const char *label, const char *filename, Soundfile **sf_zone) override {}

protected:
  void AddOrUpdateParam(IParam::EParamType type, const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init = 0., FAUSTFLOAT min = 0., FAUSTFLOAT max = 0., FAUSTFLOAT step = 1.);
  
  void BuildParameterMap()
  {
    for(auto p = 0; p < NParams(); p++)
    {
      mMap.Insert(mParams.Get(p)->GetName(), mZones.Get(p)); // insert will overwrite keys with the same name
    }
    
    if(mIPlugParamStartIdx > -1 && mPlug != nullptr) // if we've already linked parameters
    {
      CreateIPlugParameters(mPlug, mIPlugParamStartIdx);
    }

    DBGMSG("Faust Params: %s\n", mName.Get());
    
    for(auto p = 0; p < NParams(); p++)
    {
      DBGMSG("%i %s\n", p, mParams.Get(p)->GetName());
    }
  }

  int FindExistingParameterWithName(const char* name);
    
  void OnUITimer(Timer& timer)
  {
    GUI::updateAllGuis();
  }
  
  std::unique_ptr<OverSampler<sample>> mOverSampler;
  WDL_String mName;
  int mNVoices;
  std::unique_ptr<::dsp> mDSP;
  MidiHandlerPtr mMidiHandler;
  std::unique_ptr<MidiUI> mMidiUI;
  WDL_PtrList<IParam> mParams;
  WDL_PtrList<FAUSTFLOAT> mZones;
  static Timer* sUITimer;
  WDL_StringKeyedArray<FAUSTFLOAT*> mMap; // map is used for setting FAUST parameters by name, also used to reconnect existing parameters
  int mIPlugParamStartIdx = -1; // if this is negative, it means there is no linking
  
  IPlugAPIBase* mPlug = nullptr;
  bool mInitialized = false;
};

END_IPLUG_NAMESPACE
