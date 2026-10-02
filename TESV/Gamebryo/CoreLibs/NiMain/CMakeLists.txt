add_library(nimain STATIC
  TrivialFunctions.cpp
  NiStream.cpp
  NiStreamLoading.cpp
  NiStreamSaving.cpp
  NiSkinPartition.cpp
  NiStreamStatics.cpp
  NiTMapBase.cpp
  NiTArray.cpp
  NiTLargeArray.cpp
  NiRefObject.cpp
  NiExtraData.cpp
  NiObject.cpp
  NiRTTI.cpp
  NiColor.cpp
  NiPoint3.cpp
  NiMath.cpp
  NiTransform.cpp
  NiPlane.cpp
  NiFrustum.cpp
  NiQuaternion.cpp
)
target_include_directories(nimain PUBLIC ${TESV_ROOT})
target_link_libraries(nimain PUBLIC bscore nisystem)
