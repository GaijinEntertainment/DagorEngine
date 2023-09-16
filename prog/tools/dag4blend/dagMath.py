from mathutils import Matrix


def mul(mat1, mat2):
        out = Matrix()
        out[0].x = mat1[0].x * mat2[0].x + mat1[0].y * mat2[1].x + mat1[0].z * mat2[2].x + mat1[0].w * mat2[3].x
        out[0].y = mat1[0].x * mat2[0].y + mat1[0].y * mat2[1].y + mat1[0].z * mat2[2].y + mat1[0].w * mat2[3].y
        out[0].z = mat1[0].x * mat2[0].z + mat1[0].y * mat2[1].z + mat1[0].z * mat2[2].z + mat1[0].w * mat2[3].z
        out[0].w = mat1[0].x * mat2[0].w + mat1[0].y * mat2[1].w + mat1[0].z * mat2[2].w + mat1[0].w * mat2[3].w

        out[1].x = mat1[1].x * mat2[0].x + mat1[1].y * mat2[1].x + mat1[1].z * mat2[2].x + mat1[1].w * mat2[3].x
        out[1].y = mat1[1].x * mat2[0].y + mat1[1].y * mat2[1].y + mat1[1].z * mat2[2].y + mat1[1].w * mat2[3].y
        out[1].z = mat1[1].x * mat2[0].z + mat1[1].y * mat2[1].z + mat1[1].z * mat2[2].z + mat1[1].w * mat2[3].z
        out[1].w = mat1[1].x * mat2[0].w + mat1[1].y * mat2[1].w + mat1[1].z * mat2[2].w + mat1[1].w * mat2[3].w

        out[2].x = mat1[2].x * mat2[0].x + mat1[2].y * mat2[1].x + mat1[2].z * mat2[2].x + mat1[2].w * mat2[3].x
        out[2].y = mat1[2].x * mat2[0].y + mat1[2].y * mat2[1].y + mat1[2].z * mat2[2].y + mat1[2].w * mat2[3].y
        out[2].z = mat1[2].x * mat2[0].z + mat1[2].y * mat2[1].z + mat1[2].z * mat2[2].z + mat1[2].w * mat2[3].z
        out[2].w = mat1[2].x * mat2[0].w + mat1[2].y * mat2[1].w + mat1[2].z * mat2[2].w + mat1[2].w * mat2[3].w

        out[3].x = mat1[3].x * mat2[0].x + mat1[3].y * mat2[1].x + mat1[3].z * mat2[2].x + mat1[3].w * mat2[3].x
        out[3].y = mat1[3].x * mat2[0].y + mat1[3].y * mat2[1].y + mat1[3].z * mat2[2].y + mat1[3].w * mat2[3].y
        out[3].z = mat1[3].x * mat2[0].z + mat1[3].y * mat2[1].z + mat1[3].z * mat2[2].z + mat1[3].w * mat2[3].z
        out[3].w = mat1[3].x * mat2[0].w + mat1[3].y * mat2[1].w + mat1[3].z * mat2[2].w + mat1[3].w * mat2[3].w
        return out
