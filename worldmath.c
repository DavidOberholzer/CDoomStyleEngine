// External Includes
#include <math.h>

#include "structures.h"

float DotProduct(float vx1, float vy1, float vx2, float vy2)
{
    return (vx1 * vx2 + vy1 * vy2);
}

float CrossProduct(float vx1, float vy1, float vx2, float vy2)
{
    return (vx1 * vy2 - vx2 * vy1);
}

float SideOfLine(float px, float py, float x1, float y1, float x2, float y2)
{
    return CrossProduct(x2 - x1, y2 - y1, px - x1, py - y1);
}

float Max(float x, float y)
{
    return x >= y ? x : y;
}

float Min(float x, float y)
{
    return x <= y ? x : y;
}

int Clamp(int top, int bottom, float num)
{
    return Max(Min(top, num), bottom);
}

int RangesOverlap(float a0, float a1, float b0, float b1)
{
    return ((Max(a0, a1) >= Min(b0, b1)) && (Min(a0, a1) <= Max(b0, b1)));
}

int BoxesOverlap(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    return (RangesOverlap(x1, x2, x3, x4) && RangesOverlap(y1, y2, y3, y4));
}

float *VectorProjection(float x1, float y1, float x2, float y2)
{
    static float ret[2];
    ret[0] = x2 * DotProduct(x1, y1, x2, y2) / (x2 * x2 + y2 * y2);
    ret[1] = y2 * DotProduct(x1, y1, x2, y2) / (x2 * x2 + y2 * y2);
    return ret;
}

float *IntersectionPoint(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    static float ret[3];
    float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (d == 0)
    {
        ret[0] = 0;
        ret[1] = 0;
        ret[2] = 0;
    }
    else
    {
        ret[0] = (CrossProduct(x1, y1, x2, y2) * (x3 - x4) - (x1 - x2) * CrossProduct(x3, y3, x4, y4)) / d;
        ret[1] = (CrossProduct(x1, y1, x2, y2) * (y3 - y4) - (y1 - y2) * CrossProduct(x3, y3, x4, y4)) / d;
        ret[2] = 1;
    }
    return ret;
}

float *ClipBehindPlayer(float x1, float y1, float x2, float y2)
{
    static float ret[4];
    if (x1 < 0 && x2 < 0)
    {
        ret[0] = -1;
        ret[1] = -1;
        ret[2] = -1;
        ret[3] = -1;
        return ret;
    }
    if (x1 < 0)
    {
        float t = (x2 - 0.01) / (x2 - x1);
        x1 = 0.01;
        y1 = y2 * (1 - t) + y1 * t;
    }
    else if (x2 < 0)
    {
        float t = (x1 - 0.01) / (x1 - x2);
        x2 = 0.01;
        y2 = y1 * (1 - t) + y2 * t;
    }
    ret[0] = x1;
    ret[1] = y1;
    ret[2] = x2;
    ret[3] = y2;
    return ret;
}

float *ClipViewCone(float x1, float y1, float x2, float y2, float angle)
{
    // Top cone line
    float x3 = 1;
    float tangle = tan(angle);
    float y3 = x3 * -tangle;
    float *p1;
    static float ret[4];

    p1 = IntersectionPoint(x1, y1, x2, y2, 0.0, 0.0, x3, y3);

    if (*(p1 + 2) == 1)
    {
        if (*p1 > 0)
        {
            if (!((x1 < -(y1 / tangle)) && (x2 < -(y2 / tangle))))
            {
                if (x1 < -(y1 / tangle))
                {
                    x1 = *p1;
                    y1 = *(p1 + 1);
                }
                if (x2 < -(y2 / tangle))
                {
                    x2 = *p1;
                    y2 = *(p1 + 1);
                }
            }
            else
            {
                ret[0] = -1;
                ret[1] = -1;
                ret[2] = -1;
                ret[3] = -1;
                return ret;
            }
        }
    }

    // Bottom cone line
    y3 = x3 * tangle;
    float *p2;

    p2 = IntersectionPoint(x1, y1, x2, y2, 0, 0, x3, y3);
    if (*(p2 + 2) == 1)
    {
        if (*p2 > 0)
        {
            if (!((x1 < (y1 / tangle)) && (x2 < (y2 / tangle))))
            {
                if (x1 < (y1 / tangle))
                {
                    x1 = *p2;
                    y1 = *(p2 + 1);
                }
                if (x2 < (y2 / tangle))
                {
                    x2 = *p2;
                    y2 = *(p2 + 1);
                }
            }
            else
            {
                ret[0] = -1;
                ret[1] = -1;
                ret[2] = -1;
                ret[3] = -1;
                return ret;
            }
        }
    }

    ret[0] = x1;
    ret[1] = y1;
    ret[2] = x2;
    ret[3] = y2;
    return ret;
}

struct poly_line GetPolyLine(float x1, float y1, float x2, float y2, int u1, int v1, int u2, int v2, float angle)
{
    struct poly_line line;
    float *points;
    points = ClipBehindPlayer(x1, y1, x2, y2);
    if (*points > 0 && *(points + 2) > 0)
    {
        int du = u2 - u1,
            dv = v2 - v1;
        float dxb = x2 - x1,
              dyb = y2 - y1,
              dxa = *(points + 2) - *points,
              dya = *(points + 3) - *(points + 1),
              ratio;
        ratio = sqrt(dxa * dxa + dya * dya) / sqrt(dxb * dxb + dyb * dyb);
        if (*points != x1 || *(points + 1) != y1)
        {
            if (u2 > u1 || v2 > v1)
            {
                ratio = 1 - ratio;
            }
            u1 = du ? fabs(du * ratio) : u1;
            v1 = dv ? fabs(dv * ratio) : v1;
            line = (struct poly_line){*points, *(points + 1), *(points + 2), *(points + 3), u1, v1, u2, v2, 0};
        }
        else if (*(points + 2) != x2 || *(points + 3) != y2)
        {
            if (u1 > u2 || v1 > v2)
            {
                ratio = 1 - ratio;
            }
            u2 = du ? fabs(du * ratio) : u2;
            v2 = dv ? fabs(dv * ratio) : v2;
            line = (struct poly_line){*points, *(points + 1), *(points + 2), *(points + 3), u1, v1, u2, v2, 0};
        }
        else
        {
            line = (struct poly_line){*points, *(points + 1), *(points + 2), *(points + 3), u1, v1, u2, v2, 0};
        }
        return line;
    }
    return (struct poly_line){0, 0, 0, 0, 0, 0, 0, 0, 1};
}