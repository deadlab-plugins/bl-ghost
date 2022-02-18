set(BLUELAB_DEPS ${IPLUG2_DIR}/bl-libs)

add_library(_flac INTERFACE)

iplug_target_add(_flac INTERFACE
  INCLUDE ${BLUELAB_DEPS}/flac/include
  LINK ${BLUELAB_DEPS}/flac/cmake-build/src/libFLAC/libFLAC.a
)
