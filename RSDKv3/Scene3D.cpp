#include "RetroEngine.hpp"
#include <cstdlib>

int vertexCount = 0;
int faceCount   = 0;

Matrix matFinal = {};
Matrix matWorld = {};
Matrix matView  = {};
Matrix matTemp  = {};

Face faceBuffer[FACEBUFFER_SIZE];
Vertex vertexBuffer[VERTEXBUFFER_SIZE];
Vertex vertexBufferT[VERTEXBUFFER_SIZE];

DrawListEntry3D drawList3D[FACEBUFFER_SIZE];

int projectionX = 136;
int projectionY = 160;

int faceLineStart[SCREEN_YSIZE];
int faceLineEnd[SCREEN_YSIZE];
int faceLineStartU[SCREEN_YSIZE];
int faceLineEndU[SCREEN_YSIZE];
int faceLineStartV[SCREEN_YSIZE];
int faceLineEndV[SCREEN_YSIZE];

void SetIdentityMatrix(Matrix *matrix)
{
    matrix->values[0][0] = 0x100;
    matrix->values[0][1] = 0;
    matrix->values[0][2] = 0;
    matrix->values[0][3] = 0;
    matrix->values[1][0] = 0;
    matrix->values[1][1] = 0x100;
    matrix->values[1][2] = 0;
    matrix->values[1][3] = 0;
    matrix->values[2][0] = 0;
    matrix->values[2][1] = 0;
    matrix->values[2][2] = 0x100;
    matrix->values[2][3] = 0;
    matrix->values[3][0] = 0;
    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void MatrixMultiply(Matrix *matrixA, Matrix *matrixB)
{
    int output[4][4];
    int (*a)[4] = matrixA->values;
    int (*b)[4] = matrixB->values;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            output[row][col] = (a[row][0] * b[0][col] >> 8) + (a[row][1] * b[1][col] >> 8)
                             + (a[row][2] * b[2][col] >> 8) + (a[row][3] * b[3][col] >> 8);
        }
    }

    memcpy(matrixA->values, output, sizeof(output));
}
void MatrixTranslateXYZ(Matrix *matrix, int x, int y, int z)
{
    matrix->values[0][0] = 0x100;
    matrix->values[0][1] = 0;
    matrix->values[0][2] = 0;
    matrix->values[0][3] = 0;
    matrix->values[1][0] = 0;
    matrix->values[1][1] = 0x100;
    matrix->values[1][2] = 0;
    matrix->values[1][3] = 0;
    matrix->values[2][0] = 0;
    matrix->values[2][1] = 0;
    matrix->values[2][2] = 0x100;
    matrix->values[2][3] = 0;
    matrix->values[3][0] = x;
    matrix->values[3][1] = y;
    matrix->values[3][2] = z;
    matrix->values[3][3] = 0x100;
}
void MatrixScaleXYZ(Matrix *matrix, int scaleX, int scaleY, int scaleZ)
{
    matrix->values[0][0] = scaleX;
    matrix->values[0][1] = 0;
    matrix->values[0][2] = 0;
    matrix->values[0][3] = 0;
    matrix->values[1][0] = 0;
    matrix->values[1][1] = scaleY;
    matrix->values[1][2] = 0;
    matrix->values[1][3] = 0;
    matrix->values[2][0] = 0;
    matrix->values[2][1] = 0;
    matrix->values[2][2] = scaleZ;
    matrix->values[2][3] = 0;
    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void MatrixRotateX(Matrix *matrix, int rotationX)
{
    if (rotationX < 0)
        rotationX = 0x200 - rotationX;
    rotationX &= 0x1FF;
    int sine             = Sin512(rotationX) >> 1;
    int cosine           = Cos512(rotationX) >> 1;
    matrix->values[0][0] = 0x100;
    matrix->values[0][1] = 0;
    matrix->values[0][2] = 0;
    matrix->values[0][3] = 0;
    matrix->values[1][0] = 0;
    matrix->values[1][1] = cosine;
    matrix->values[1][2] = sine;
    matrix->values[1][3] = 0;
    matrix->values[2][0] = 0;
    matrix->values[2][1] = -sine;
    matrix->values[2][2] = cosine;
    matrix->values[2][3] = 0;
    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void MatrixRotateY(Matrix *matrix, int rotationY)
{
    if (rotationY < 0)
        rotationY = 0x200 - rotationY;
    rotationY &= 0x1FF;
    int sine             = Sin512(rotationY) >> 1;
    int cosine           = Cos512(rotationY) >> 1;
    matrix->values[0][0] = cosine;
    matrix->values[0][1] = 0;
    matrix->values[0][2] = sine;
    matrix->values[0][3] = 0;
    matrix->values[1][0] = 0;
    matrix->values[1][1] = 0x100;
    matrix->values[1][2] = 0;
    matrix->values[1][3] = 0;
    matrix->values[2][0] = -sine;
    matrix->values[2][1] = 0;
    matrix->values[2][2] = cosine;
    matrix->values[2][3] = 0;
    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void MatrixRotateZ(Matrix *matrix, int rotationZ)
{
    if (rotationZ < 0)
        rotationZ = 0x200 - rotationZ;
    rotationZ &= 0x1FF;
    int sine             = Sin512(rotationZ) >> 1;
    int cosine           = Cos512(rotationZ) >> 1;
    matrix->values[0][0] = cosine;
    matrix->values[0][1] = 0;
    matrix->values[0][2] = sine;
    matrix->values[0][3] = 0;
    matrix->values[1][0] = 0;
    matrix->values[1][1] = 0x100;
    matrix->values[1][2] = 0;
    matrix->values[1][3] = 0;
    matrix->values[2][0] = -sine;
    matrix->values[2][1] = 0;
    matrix->values[2][2] = cosine;
    matrix->values[2][3] = 0;
    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void MatrixRotateXYZ(Matrix *matrix, int rotationX, int rotationY, int rotationZ)
{
    if (rotationX < 0)
        rotationX = 0x200 - rotationX;
    rotationX &= 0x1FF;
    if (rotationY < 0)
        rotationY = 0x200 - rotationY;
    rotationY &= 0x1FF;
    if (rotationZ < 0)
        rotationZ = 0x200 - rotationZ;
    rotationZ &= 0x1FF;
    int sineX   = Sin512(rotationX) >> 1;
    int cosineX = Cos512(rotationX) >> 1;
    int sineY   = Sin512(rotationY) >> 1;
    int cosineY = Cos512(rotationY) >> 1;
    int sineZ   = Sin512(rotationZ) >> 1;
    int cosineZ = Cos512(rotationZ) >> 1;

    matrix->values[0][0] = (sineZ * (sineY * sineX >> 8) >> 8) + (cosineZ * cosineY >> 8);
    matrix->values[0][1] = (sineZ * cosineY >> 8) - (cosineZ * (sineY * sineX >> 8) >> 8);
    matrix->values[0][2] = sineY * cosineX >> 8;
    matrix->values[0][3] = 0;
    matrix->values[1][0] = sineZ * -cosineX >> 8;
    matrix->values[1][1] = cosineZ * cosineX >> 8;
    matrix->values[1][2] = sineX;
    matrix->values[1][3] = 0;
    matrix->values[2][0] = (sineZ * (cosineY * sineX >> 8) >> 8) - (cosineZ * sineY >> 8);
    matrix->values[2][1] = (sineZ * -sineY >> 8) - (cosineZ * (cosineY * sineX >> 8) >> 8);
    matrix->values[2][2] = cosineY * cosineX >> 8;
    matrix->values[2][3] = 0;
    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void TransformVertexBuffer()
{
    matFinal = matWorld;
    MatrixMultiply(&matFinal, &matView);

    if (vertexCount <= 0)
        return;

    int m00 = matFinal.values[0][0], m01 = matFinal.values[0][1], m02 = matFinal.values[0][2];
    int m10 = matFinal.values[1][0], m11 = matFinal.values[1][1], m12 = matFinal.values[1][2];
    int m20 = matFinal.values[2][0], m21 = matFinal.values[2][1], m22 = matFinal.values[2][2];
    int m30 = matFinal.values[3][0], m31 = matFinal.values[3][1], m32 = matFinal.values[3][2];

    Vertex *src = vertexBuffer;
    Vertex *dst = vertexBufferT;
    for (int i = 0; i < vertexCount; ++i, ++src, ++dst) {
        int vx = src->x, vy = src->y, vz = src->z;
        dst->x = (vx * m00 >> 8) + (vy * m10 >> 8) + (vz * m20 >> 8) + m30;
        dst->y = (vx * m01 >> 8) + (vy * m11 >> 8) + (vz * m21 >> 8) + m31;
        dst->z = (vx * m02 >> 8) + (vy * m12 >> 8) + (vz * m22 >> 8) + m32;
    }
}
void TransformVerticies(Matrix *matrix, int startIndex, int endIndex)
{
    if (startIndex >= endIndex)
        return;

    int m00 = matrix->values[0][0], m01 = matrix->values[0][1], m02 = matrix->values[0][2];
    int m10 = matrix->values[1][0], m11 = matrix->values[1][1], m12 = matrix->values[1][2];
    int m20 = matrix->values[2][0], m21 = matrix->values[2][1], m22 = matrix->values[2][2];
    int m30 = matrix->values[3][0], m31 = matrix->values[3][1], m32 = matrix->values[3][2];

    Vertex *vert = &vertexBuffer[startIndex];
    for (int i = startIndex; i < endIndex; ++i, ++vert) {
        int vx = vert->x, vy = vert->y, vz = vert->z;
        vert->x = (vx * m00 >> 8) + (vy * m10 >> 8) + (vz * m20 >> 8) + m30;
        vert->y = (vx * m01 >> 8) + (vy * m11 >> 8) + (vz * m21 >> 8) + m31;
        vert->z = (vx * m02 >> 8) + (vy * m12 >> 8) + (vz * m22 >> 8) + m32;
    }
}
static int CompareDrawList3D(const void *a, const void *b)
{
    return ((DrawListEntry3D*)b)->depth - ((DrawListEntry3D*)a)->depth;
}

void Sort3DDrawList()
{
    for (int i = 0; i < faceCount; ++i) {
        drawList3D[i].depth = (vertexBufferT[faceBuffer[i].d].z + vertexBufferT[faceBuffer[i].c].z + vertexBufferT[faceBuffer[i].b].z
                               + vertexBufferT[faceBuffer[i].a].z)
                              >> 2;
        drawList3D[i].faceID = i;
    }

    qsort(drawList3D, faceCount, sizeof(DrawListEntry3D), CompareDrawList3D);
}
void Draw3DScene(int spriteSheetID)
{
    Vertex quad[4];
    int pX = projectionX;
    int pY = projectionY;
    int cx = SCREEN_CENTERX;
    int cy = SCREEN_CENTERY;
    
    for (int i = 0; i < faceCount; ++i) {
        Face *face = &faceBuffer[drawList3D[i].faceID];
        switch (face->flags) {
            default: break;
            case FACE_FLAG_TEXTURED_3D: {
                Vertex *va = &vertexBufferT[face->a];
                Vertex *vb = &vertexBufferT[face->b];
                Vertex *vc = &vertexBufferT[face->c];
                Vertex *vd = &vertexBufferT[face->d];
                if (va->z > 0x100 && vb->z > 0x100 && vc->z > 0x100 && vd->z > 0x100) {
                    quad[0].x = cx + pX * va->x / va->z;
                    quad[0].y = cy - pY * va->y / va->z;
                    quad[1].x = cx + pX * vb->x / vb->z;
                    quad[1].y = cy - pY * vb->y / vb->z;
                    quad[2].x = cx + pX * vc->x / vc->z;
                    quad[2].y = cy - pY * vc->y / vc->z;
                    quad[3].x = cx + pX * vd->x / vd->z;
                    quad[3].y = cy - pY * vd->y / vd->z;
                    quad[0].u = vertexBuffer[face->a].u;
                    quad[0].v = vertexBuffer[face->a].v;
                    quad[1].u = vertexBuffer[face->b].u;
                    quad[1].v = vertexBuffer[face->b].v;
                    quad[2].u = vertexBuffer[face->c].u;
                    quad[2].v = vertexBuffer[face->c].v;
                    quad[3].u = vertexBuffer[face->d].u;
                    quad[3].v = vertexBuffer[face->d].v;
                    DrawTexturedFace(quad, spriteSheetID);
                }
                break;
            }
            case FACE_FLAG_TEXTURED_2D:
                quad[0].x = vertexBuffer[face->a].x;
                quad[0].y = vertexBuffer[face->a].y;
                quad[1].x = vertexBuffer[face->b].x;
                quad[1].y = vertexBuffer[face->b].y;
                quad[2].x = vertexBuffer[face->c].x;
                quad[2].y = vertexBuffer[face->c].y;
                quad[3].x = vertexBuffer[face->d].x;
                quad[3].y = vertexBuffer[face->d].y;
                quad[0].u = vertexBuffer[face->a].u;
                quad[0].v = vertexBuffer[face->a].v;
                quad[1].u = vertexBuffer[face->b].u;
                quad[1].v = vertexBuffer[face->b].v;
                quad[2].u = vertexBuffer[face->c].u;
                quad[2].v = vertexBuffer[face->c].v;
                quad[3].u = vertexBuffer[face->d].u;
                quad[3].v = vertexBuffer[face->d].v;
                DrawTexturedFace(quad, spriteSheetID);
                break;
            case FACE_FLAG_COLOURED_3D: {
                Vertex *va = &vertexBufferT[face->a];
                Vertex *vb = &vertexBufferT[face->b];
                Vertex *vc = &vertexBufferT[face->c];
                Vertex *vd = &vertexBufferT[face->d];
                if (va->z > 0x100 && vb->z > 0x100 && vc->z > 0x100 && vd->z > 0x100) {
                    quad[0].x = cx + pX * va->x / va->z;
                    quad[0].y = cy - pY * va->y / va->z;
                    quad[1].x = cx + pX * vb->x / vb->z;
                    quad[1].y = cy - pY * vb->y / vb->z;
                    quad[2].x = cx + pX * vc->x / vc->z;
                    quad[2].y = cy - pY * vc->y / vc->z;
                    quad[3].x = cx + pX * vd->x / vd->z;
                    quad[3].y = cy - pY * vd->y / vd->z;
                    DrawFace(quad, face->colour);
                }
                break;
            }
            case FACE_FLAG_COLOURED_2D:
                quad[0].x = vertexBuffer[face->a].x;
                quad[0].y = vertexBuffer[face->a].y;
                quad[1].x = vertexBuffer[face->b].x;
                quad[1].y = vertexBuffer[face->b].y;
                quad[2].x = vertexBuffer[face->c].x;
                quad[2].y = vertexBuffer[face->c].y;
                quad[3].x = vertexBuffer[face->d].x;
                quad[3].y = vertexBuffer[face->d].y;
                DrawFace(quad, face->colour);
                break;
        }
    }
}

void ProcessScanEdge(Vertex *vertA, Vertex *vertB)
{
    int bottom, top;
    int fullX, fullY;
    int trueX, yDifference;

    if (vertA->y == vertB->y)
        return;
    if (vertA->y >= vertB->y) {
        top    = vertB->y;
        bottom = vertA->y + 1;
    }
    else {
        top    = vertA->y;
        bottom = vertB->y + 1;
    }
    if (top > SCREEN_YSIZE - 1 || bottom < 0)
        return;
    if (bottom > SCREEN_YSIZE)
        bottom = SCREEN_YSIZE;
    fullX       = vertA->x << 16;
    yDifference = vertB->y - vertA->y;
    fullY       = ((vertB->x - vertA->x) << 16) / yDifference;
    if (top < 0) {
        fullX -= top * fullY;
        top = 0;
    }
    for (int i = top; i < bottom; ++i) {
        trueX = fullX >> 16;
        if (fullX >> 16 < faceLineStart[i])
            faceLineStart[i] = trueX;
        if (trueX > faceLineEnd[i])
            faceLineEnd[i] = trueX;
        fullX += fullY;
    }
}
void ProcessScanEdgeUV(Vertex *vertA, Vertex *vertB)
{
    int bottom, top;
    int fullX, fullU, fullV;
    int trueX, trueU, trueV, yDifference;

    if (vertA->y == vertB->y)
        return;
    if (vertA->y >= vertB->y) {
        top    = vertB->y;
        bottom = vertA->y + 1;
    }
    else {
        top    = vertA->y;
        bottom = vertB->y + 1;
    }
    if (top > SCREEN_YSIZE - 1 || bottom < 0)
        return;
    if (bottom > SCREEN_YSIZE)
        bottom = SCREEN_YSIZE;
    fullX      = vertA->x << 16;
    fullU      = vertA->u << 16;
    fullV      = vertA->v << 16;
    int finalX = ((vertB->x - vertA->x) << 16) / (vertB->y - vertA->y);
    if (vertA->u == vertB->u)
        trueU = 0;
    else
        trueU = ((vertB->u - vertA->u) << 16) / (vertB->y - vertA->y);

    if (vertA->v == vertB->v) {
        trueV = 0;
    }
    else {
        yDifference = vertB->y - vertA->y;
        trueV       = ((vertB->v - vertA->v) << 16) / yDifference;
    }
    if (top < 0) {
        fullX -= top * finalX;
        fullU -= top * trueU;
        fullV -= top * trueV;
        top = 0;
    }
    for (int i = top; i < bottom; ++i) {
        trueX = fullX >> 16;
        if (fullX >> 16 < faceLineStart[i]) {
            faceLineStart[i]  = trueX;
            faceLineStartU[i] = fullU;
            faceLineStartV[i] = fullV;
        }
        if (trueX > faceLineEnd[i]) {
            faceLineEnd[i]  = trueX;
            faceLineEndU[i] = fullU;
            faceLineEndV[i] = fullV;
        }
        fullX += finalX;
        fullU += trueU;
        fullV += trueV;
    }
}
