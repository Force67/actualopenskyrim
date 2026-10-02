add_library(nimain STATIC
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
)
target_include_directories(nimain PUBLIC ${TESV_ROOT})
target_link_libraries(nimain PUBLIC bscore)
