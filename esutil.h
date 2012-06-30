#ifndef _ESUTIL_H_
#define _ESUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
   float m[4][4];
} ESMatrix;

typedef struct
{
   float m[3][3];
} ESMatrix3;

void esTranslate(ESMatrix *result, float tx, float ty, float tz);
void esScale(ESMatrix *result, float sx, float sy, float sz);
void esMatrixMultiply(ESMatrix *result, const ESMatrix *srcA, const ESMatrix *srcB);
int esInverse(ESMatrix * in, ESMatrix * out);
void esRotate(ESMatrix *result, float angle, float x, float y, float z);
void esMatrixLoadIdentity(ESMatrix *result);
void esFrustum(ESMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ);
void esPerspective(ESMatrix *result, float fovy, float aspect, float zNear, float zFar);
void esOrtho(ESMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ);
void esLookAt(ESMatrix *result, float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz);

#ifdef __cplusplus
}
#endif

#endif /* _ESUTIL_H_ */

