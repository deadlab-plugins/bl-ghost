set(BLUELAB_DEPS ${IPLUG2_DIR}/bl-libs)

add_library(_bluelab-lib INTERFACE)
set(BLUELAB_LIB_SRC "${BLUELAB_DEPS}/bluelab-lib/")
set(_bluelab-lib_src
  AudioFile.cpp
  BlaTimer.cpp
  BLProfiler.cpp
  BLSpectrogram4.cpp
  BLTransport.cpp
  BLUtilsAlgo.cpp
  BLUtilsComp.cpp
  BLUtils.cpp
  BLUtilsDecim.cpp
  BLUtilsFade.cpp
  BLUtilsFft.cpp
  BLUtilsFile.cpp
  BLUtilsMath.cpp
  BLUtilsPhases.cpp
  BLUtilsPlug.cpp
  BLVumeter2SidesControl.cpp
  BLVumeterControl.cpp
  BLVumeterNeedleControl.cpp
  CMA2Smoother.cpp
  CMAParamSmooth2.cpp
  CMAParamSmooth.cpp
  CMASmoother.cpp
  ColorMap4.cpp
  ColorMapFactory.cpp
  DemoModeManager.cpp
  FftProcessObj16.cpp
  FilterBank.cpp
  GhostCommandCopyPaste.cpp
  GhostCommand.cpp
  GhostCommandCut.cpp
  GhostCommandGain.cpp
  GhostCommandHistory.cpp
  GhostCommandReplace.cpp
  GhostCustomControl.cpp
  GhostCustomDrawer.cpp
  GhostPluginInterface.cpp
  GhostTriggerControl.cpp
  GraphAxis2.cpp
  GraphControl12.cpp
  GraphCurve5.cpp
  GraphFreqAxis2.cpp
  GraphTimeAxis6.cpp
  GUIHelper12.cpp
  IBitmapControlAnim.cpp
  IBLSwitchControl.cpp
  IGUIResizeButtonControl.cpp
  IHelpButtonControl2.cpp
  IHelpButtonControl.cpp
  IIconLabelControl.cpp
  ImageDisplay.cpp
  ImageInpaint2.cpp
  IRadioButtonsControl.cpp
  IRadioButtonsControlCustom.cpp
  IRolloverButtonControl.cpp
  IsophoteInpaint.cpp
  ISpatializerHandleControl.cpp
  ITabsBarControl.cpp
  IXYPadControl.cpp
  IXYPadControlExt.cpp
  MelScale.cpp
  MiniView2.cpp
  MiniView.cpp
  ParamSmoother.cpp
  PPMFile.cpp
  Resampler2.cpp
  ResizeGUIPluginInterface.cpp
  SamplesPyramid3.cpp
  SamplesToMagnPhases.cpp
  Scale.cpp
  SecureRestarter.cpp
  SimpleInpaintPolar3.cpp
  SpectroEditFftObj3.cpp
  SpectrogramDisplay2.cpp
  SpectrogramDisplay3.cpp
  SpectrogramDisplay.cpp
  SpectrogramDisplayScroll2.cpp
  SpectrogramDisplayScroll.cpp
  SpectrogramFftObj2.cpp
  SpectrogramView2.cpp
  SpectroMeter.cpp
  UpTime.cpp
  Window.cpp
  )
list(TRANSFORM _bluelab-lib_src PREPEND "${BLUELAB_LIB_SRC}")
iplug_target_add(_bluelab-lib INTERFACE
  INCLUDE ${BLUELAB_DEPS}/bluelab-lib
  SOURCE ${_bluelab-lib_src}
  #DEFINE "REAPER_PLUGIN"
  #LINK iPlug2_VST2
)
