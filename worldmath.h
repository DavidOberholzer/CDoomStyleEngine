#ifndef WORLDMATH_INCLUDED
#define WORLDMATH_INCLUDED

float DotProduct(float, float, float, float);

float CrossProduct(float, float, float, float);

float SideOfLine(float, float, float, float, float, float);

float Max(float, float);

float Min(float, float);

int Clamp(int, int, float);

int RangesOverlap(float, float, float, float);

int BoxesOverlap(float, float, float, float, float, float, float, float);

float *VectorProjection(float, float, float, float);

float *IntersectionPoint(float, float, float, float, float, float, float, float);

float *ClipViewCone(float, float, float, float, float);

#endif