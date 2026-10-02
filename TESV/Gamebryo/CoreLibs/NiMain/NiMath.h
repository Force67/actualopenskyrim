#pragma once

// Tables filled by CreateSinTable, 512 entries covering [0, 2*pi).
extern float s_fSinTableA[512];
extern float s_fCosTableA[512];

void CreateSinTable();
float NiFastATan2(float fY, float fX);
