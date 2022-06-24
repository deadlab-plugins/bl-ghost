<<includeIntrinsic>>

#define FAUSTFLOAT iplug::sample

#include "IPlugConstants.h"
#include "IPlugFaust.h"

/* BEGIN AUTO GENERATED BY THE FAUST COMPILER ... */
<<includeclass>>
/* ... END AUTO GENERATED BY THE FAUST COMPILER  */

using namespace iplug;

class Faust_mydsp : public IPlugFaust
{
public:
  Faust_mydsp(const char* name,
              const char* inputDSPFile = 0,
              int nVoices = 1,
              int rate = 1,
              const char* outputCPPFile = 0,
              const char* drawPath = 0,
              const char* libraryPath = FAUST_SHARE_PATH)
  : IPlugFaust(name, nVoices)
  {
  }

  void Init() override
  {
    mMidiHandler = std::make_unique<iplug2_midi_handler>();
    mMidiUI = std::make_unique<MidiUI>(mMidiHandler.get());
      
    ::dsp* tmpDsp = new FAUSTCLASS();
    
    // Polyphony handling
    bool midiSync = false;
    int nvoices = 0;
    MidiMeta::analyse(tmpDsp, midiSync, nvoices);
    
    if (nvoices > 0)
    {
      dsp_poly* dspPoly = new FAUSTCLASS_POLY(tmpDsp, nvoices, true);
      mMidiHandler->addMidiIn(dspPoly);
      mDSP = std::unique_ptr<::dsp>(dspPoly);
    }
    else
    {
      mDSP = std::unique_ptr<::dsp>(tmpDsp);
    }
    
    // MIDI handling
    mDSP->buildUserInterface(mMidiUI.get());
    mDSP->buildUserInterface(this);
    
    BuildParameterMap();
    
    mInitialized = true;
  }
};

#undef FAUSTCLASS
