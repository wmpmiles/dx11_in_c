#include <intrin.h>

typedef union {
    struct {
        float x;
        float y;
        float z;
        float w;
    };
    _Alignas(16) float values[4];
} RawF4;
_Static_assert(_Alignof(RawF4) >= 16, "");

typedef union {
    struct {
        RawF4 x;
        RawF4 y;
        RawF4 z;
        RawF4 w;
    };
    _Alignas(64) float values[16];
} RawF16;
_Static_assert(_Alignof(RawF16) >= 64, "");

typedef struct {
    __m128 value;
} VecF4;

typedef union {
    struct {
        VecF4 x;
        VecF4 y;
        VecF4 z;
        VecF4 w;
    };
    VecF4 values[4];
} MatF4x4;

//
// === CONSTANTS ===
//

#define VF4_ZERO (vf4_from_rf4((RawF4){ 0.0f, 0.0f, 0.0f, 0.0f }))
#define VF4_E0 (vf4_from_rf4((RawF4){ 1.0f, 0.0f, 0.0f, 0.0f }))
#define VF4_E1 (vf4_from_rf4((RawF4){ 0.0f, 1.0f, 0.0f, 0.0f }))
#define VF4_E2 (vf4_from_rf4((RawF4){ 0.0f, 0.0f, 1.0f, 0.0f }))
#define VF4_E3 (vf4_from_rf4((RawF4){ 0.0f, 0.0f, 0.0f, 1.0f }))

//
// === MACRO FUNCTIONS ===
//

#define VF4_FROM(x, y, z, w) (vf4_from_rf4((RawF4){x, y, z, w}))

#define VF4_MASK4(b0, b1, b2, b3) ((unsigned char)(b0 << 0 | b1 << 1 | b2 << 2 | b3 << 3))
#define VF4_EXTRACT(vec, mask) ((VecF4){ _mm_blend_ps(VF4_ZERO.value, vec.value, mask) })
#define VF4_MOVE(dst, src, mask) ((VecF4){ _mm_blend_ps(dst.value, src.value, mask) })

#define VF4_SHUFFLE_CTRL(a, b, c, d) (a << 0 | b << 2 | c << 4 | d << 6)
#define VF4_SHUFFLE(vec, ctrl) ((VecF4){ _mm_shuffle_ps(vec.value, vec.value, ctrl) })

//
// === CONVERSION ===
//

RawF4 __vectorcall rf4_from_vf4(VecF4 vec) {
    RawF4 ret;
    _mm_store_ps(ret.values, vec.value);
    return ret;
}

VecF4 __vectorcall vf4_from_rf4(RawF4 raw) {
    return (VecF4){ _mm_load_ps(raw.values) };
}

RawF16 __vectorcall rf16_from_mf4x4(MatF4x4 mat) {
    return (RawF16) {
        rf4_from_vf4(mat.x),
        rf4_from_vf4(mat.y),
        rf4_from_vf4(mat.z),
        rf4_from_vf4(mat.w)
    };
}

MatF4x4 __vectorcall mf4x4_from_vf4(VecF4 x, VecF4 y, VecF4 z, VecF4 w) {
    return (MatF4x4){ x, y, z, w };
}

float __vectorcall vf4_ss_get(VecF4 vec) {
    return _mm_cvtss_f32(vec.value);
}

VecF4 __vectorcall vf4_ss_set(float value) {
    return (VecF4){ _mm_set_ss(value) };
}

VecF4 __vectorcall vf4_broadcast(float value) {
    return (VecF4){ _mm_set1_ps(value) };
}

//
// === ELEMENT-WISE ARITHMETIC ===
//

VecF4 __vectorcall vf4_add(VecF4 lhs, VecF4 rhs) {
    return (VecF4){ _mm_add_ps(lhs.value, rhs.value) };
}

VecF4 __vectorcall vf4_sub(VecF4 lhs, VecF4 rhs) {
    return (VecF4){ _mm_sub_ps(lhs.value, rhs.value) };
}

VecF4 __vectorcall vf4_mul(VecF4 lhs, VecF4 rhs) {
    return (VecF4){ _mm_mul_ps(lhs.value, rhs.value) };
}

VecF4 __vectorcall vf4_negate(VecF4 vec) {
    return (VecF4){ _mm_sub_ps(VF4_ZERO.value, vec.value) };
}

VecF4 __vectorcall vf4_reciprocal(VecF4 vec) {
    return (VecF4){ _mm_rcp_ps(vec.value) };
}

//
// === ELEMENT-WISE FUNCTIONS ===
//

VecF4 __vectorcall vf4_tand(VecF4 vec) {
    return (VecF4){ _mm_tand_ps(vec.value) };
}

VecF4 __vectorcall vf4_rsqrt(VecF4 vec) {
    return (VecF4){ _mm_rsqrt_ps(vec.value) };
}

VecF4 __vectorcall vf4_eq(VecF4 lhs, VecF4 rhs) {
    return (VecF4){ _mm_cmpeq_ps(lhs.value, rhs.value) };
}

VecF4 __vectorcall vf4_gt(VecF4 lhs, VecF4 rhs) {
    return (VecF4){ _mm_cmpgt_ps(lhs.value, rhs.value) };
}

VecF4 __vectorcall vf4_ge(VecF4 lhs, VecF4 rhs) {
    return (VecF4){ _mm_cmpge_ps(lhs.value, rhs.value) };
}

VecF4 __vectorcall vf4_lt(VecF4 lhs, VecF4 rhs) {
    return (VecF4){ _mm_cmplt_ps(lhs.value, rhs.value) };
}

VecF4 __vectorcall vf4_le(VecF4 lhs, VecF4 rhs) {
    return (VecF4){ _mm_cmple_ps(lhs.value, rhs.value) };
}

VecF4 __vectorcall vf4_abs(VecF4 vec) {
    const VecF4 mask = (VecF4){ _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)) };
    return (VecF4){ _mm_and_ps(mask.value, vec.value) };
}

_Bool __vectorcall vf4_epsilon_equal(VecF4 lhs, VecF4 rhs, float epsilon) {
    VecF4 scaled_epsilon = vf4_mul(vf4_abs(lhs), vf4_broadcast(epsilon));
    VecF4 lower = vf4_sub(lhs, scaled_epsilon);
    VecF4 upper = vf4_add(lhs, scaled_epsilon);
    VecF4 cmp_lower = vf4_ge(rhs, lower);
    VecF4 cmp_upper = vf4_le(rhs, upper);
    return (_mm_movemask_ps(cmp_upper.value) == 0b1111) && (_mm_movemask_ps(cmp_lower.value) == 0b1111);
}

//
// === VECTOR OPERATIONS ===
//

VecF4 __vectorcall vf4_dot(VecF4 lhs, VecF4 rhs) {
    return (VecF4){ _mm_dp_ps(lhs.value, rhs.value, 0xff) };
}

VecF4 __vectorcall vf4_normalize(VecF4 vec) {
    VecF4 norm2 = vf4_dot(vec, vec);
    VecF4 rnorm = vf4_rsqrt(norm2);
    return vf4_mul(rnorm, vec);
}

VecF4 __vectorcall vf4_cross(VecF4 lhs, VecF4 rhs) {
    __m128 ll = _mm_shuffle_ps(lhs.value, lhs.value, VF4_SHUFFLE_CTRL(1, 2, 0, 3));
    __m128 lr = _mm_shuffle_ps(rhs.value, rhs.value, VF4_SHUFFLE_CTRL(2, 0, 1, 3));
    __m128 rl = _mm_shuffle_ps(lhs.value, lhs.value, VF4_SHUFFLE_CTRL(2, 0, 1, 3));
    __m128 rr = _mm_shuffle_ps(rhs.value, rhs.value, VF4_SHUFFLE_CTRL(1, 2, 0, 3));
    __m128 l = _mm_mul_ps(ll, lr);
    __m128 r = _mm_mul_ps(rl, rr);
    return (VecF4){ _mm_sub_ps(l, r) };
}

//
// === MATRIX OPERATIONS ===

MatF4x4 __vectorcall mf4x4_transpose(MatF4x4 mat) {
    _MM_TRANSPOSE4_PS(mat.x.value, mat.y.value, mat.z.value, mat.w.value);
    return mat;
}

VecF4 __vectorcall mf4x4_element_dot_vf4(MatF4x4 mat, VecF4 vec) {
    VecF4 x = vf4_dot(mat.x, vec);
    VecF4 y = vf4_dot(mat.y, vec);
    VecF4 z = vf4_dot(mat.z, vec);
    VecF4 w = vf4_dot(mat.w, vec);
    VecF4 xy = VF4_MOVE(x, y, VF4_MASK4(0, 1, 0, 0));
    VecF4 zw = VF4_MOVE(z, w, VF4_MASK4(0, 0, 0, 1));
    return VF4_MOVE(xy, zw, VF4_MASK4(0, 0, 1, 1));
}

MatF4x4 __vectorcall mf4x4_inv_orthonormal_point(VecF4 u_x, VecF4 u_y, VecF4 u_z, VecF4 p) {
    MatF4x4 rotation = mf4x4_from_vf4(u_x, u_y, u_z, VF4_E3);
    VecF4 point = vf4_negate(mf4x4_element_dot_vf4(rotation, p));
    rotation.x = VF4_MOVE(rotation.x, VF4_SHUFFLE(point, VF4_SHUFFLE_CTRL(0, 0, 0, 0)), VF4_MASK4(0, 0, 0, 1));
    rotation.y = VF4_MOVE(rotation.y, VF4_SHUFFLE(point, VF4_SHUFFLE_CTRL(0, 0, 0, 1)), VF4_MASK4(0, 0, 0, 1));
    rotation.z = VF4_MOVE(rotation.z, VF4_SHUFFLE(point, VF4_SHUFFLE_CTRL(0, 0, 0, 2)), VF4_MASK4(0, 0, 0, 1));
    return rotation;
}
