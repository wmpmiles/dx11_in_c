#include <stdio.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof(*(a)))
#define CHECK(expr, error_str) { if (!(expr)) { return error_str; } }
#define TEST(test_name) { .fn = test_##test_name, .name = #test_name }

typedef const char *(*Test)(void);
typedef struct {
    Test fn;
    const char *name;
} TestEntry;

//
// === TESTING AREA ===
//

// From _mm_rsqrt_ps having a max relative error of 1.5 * 2 ^ -12
#define MAX_RELATIVE_ERROR 0.0003662109375f

#include "3dmath.c"

const char *test_vf4_normalize(void) {
    VecF4 input = VF4_FROM(2.0f, 2.0f, 2.0f, 2.0f);
    VecF4 expected = VF4_FROM(0.5f, 0.5f, 0.5f, 0.5f);

    VecF4 output = vf4_normalize(input);
    CHECK(vf4_epsilon_equal(output, expected, MAX_RELATIVE_ERROR), "");

    return NULL;
}

const char *test_vf4_cross(void) {
    VecF4 x_y = vf4_cross(VF4_E0, VF4_E1);
    VecF4 y_z = vf4_cross(VF4_E1, VF4_E2);
    VecF4 z_x = vf4_cross(VF4_E2, VF4_E0);

    CHECK(vf4_epsilon_equal(x_y, VF4_E2, MAX_RELATIVE_ERROR), "X x Y = Z");
    CHECK(vf4_epsilon_equal(y_z, VF4_E0, MAX_RELATIVE_ERROR), "Y x Z = X");
    CHECK(vf4_epsilon_equal(z_x, VF4_E1, MAX_RELATIVE_ERROR), "Z x X = Y");

    VecF4 y_x = vf4_cross(VF4_E1, VF4_E0);
    VecF4 z_y = vf4_cross(VF4_E2, VF4_E1);
    VecF4 x_z = vf4_cross(VF4_E0, VF4_E2);

    CHECK(vf4_epsilon_equal(y_x, vf4_negate(VF4_E2), MAX_RELATIVE_ERROR), "Y x X = -Z");
    CHECK(vf4_epsilon_equal(z_y, vf4_negate(VF4_E0), MAX_RELATIVE_ERROR), "Z x Y = -X");
    CHECK(vf4_epsilon_equal(x_z, vf4_negate(VF4_E1), MAX_RELATIVE_ERROR), "X x Z = -Y");

    return NULL;
}

const char *test_mf4x4_element_dot_vf4(void) {
    MatF4x4 m_identity = mf4x4_from_vf4(VF4_E0, VF4_E1, VF4_E2, VF4_E3);
    MatF4x4 m_reverse = mf4x4_from_vf4(VF4_E3, VF4_E2, VF4_E1, VF4_E0);
    MatF4x4 m_all_x = mf4x4_from_vf4(VF4_E0, VF4_E0, VF4_E0, VF4_E0);
    VecF4 v_vec = VF4_FROM(2, 3, 4, 5);
    VecF4 v_reverse = VF4_FROM(5, 4, 3, 2);
    VecF4 v_all_x = VF4_FROM(2, 2, 2, 2);
    
    VecF4 result;

    result = mf4x4_element_dot_vf4(m_identity, v_vec);
    CHECK(vf4_epsilon_equal(result, v_vec, 0), "element dot by identity");

    result = mf4x4_element_dot_vf4(m_reverse, v_vec);
    CHECK(vf4_epsilon_equal(result, v_reverse, 0), "element dot by element reverser");

    result = mf4x4_element_dot_vf4(m_all_x, v_vec);
    CHECK(vf4_epsilon_equal(result, v_all_x, 0), "element dot by all x");

    return NULL;
}

const char *test_mf4x4_inv_orthonormal_point(void) {
    MatF4x4 m_result = mf4x4_inv_orthonormal_point(
        VF4_FROM(0, 1, 0, 0),
        VF4_FROM(0, 0, 1, 0),
        VF4_FROM(1, 0, 0, 0),
        VF4_FROM(4, 3, 2, 1)
    );
    MatF4x4 m_expected = mf4x4_from_vf4(
        VF4_FROM(0, 1, 0, -3),
        VF4_FROM(0, 0, 1, -2),
        VF4_FROM(1, 0, 0, -4),
        VF4_FROM(0, 0, 0,  1)
    );

    for (int i = 0; i != 4; ++i) {
        CHECK(vf4_epsilon_equal(m_result.values[i], m_expected.values[i], MAX_RELATIVE_ERROR), "");
    }

    return NULL;
}

const TestEntry tests[] = {
    TEST(vf4_normalize),
    TEST(vf4_cross),
    TEST(mf4x4_element_dot_vf4),
    TEST(mf4x4_inv_orthonormal_point),
};

//
// === HARNESS ===
//

int main(void) {
    for (int i = 0; i != ARRAY_LEN(tests); ++i) {
        const char *result = tests[i].fn();
        if (result) {
            printf("FAIL (%s): %s.\n", tests[i].name, result);
        } else {
            printf("PASS (%s).\n", tests[i].name);
        }
    }
    return 0;
}
