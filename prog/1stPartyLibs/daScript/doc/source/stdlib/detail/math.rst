.. |function-math-sign| replace:: returns sign of x, or 0 if x == 0

.. |function-math-abs| replace:: returns the absolute value of x

.. |function-math-acos| replace:: returns the arccosine of x

.. |function-math-asin| replace:: returns the arcsine of x

.. |function-math-atan| replace:: returns the arctangent of x

.. |function-math-atan2| replace:: returns the arctangent of y/x

.. |function-math-atan2_est| replace:: returns the fast approximation of arctangent of y/x

.. |function-math-ceil| replace:: returns a float value representing the smallest integer (type is still float) that is greater than or equal to arg0

.. |function-math-ceili| replace:: returns a value representing the smallest integer (integer type!) that is greater than or equal to arg0

.. |function-math-clamp| replace:: returns vector or scalar representing min(max(t, a), b)

.. |function-math-cos| replace:: returns the cosine of x

.. |function-math-cross| replace:: returns vector representing cross product between x and y

.. |function-math-distance| replace:: returns a non-negative value representing distance between x and y

.. |function-math-distance_sq| replace:: returns a non-negative value representing squared distance between x and y

.. |function-math-dot| replace:: returns scalar representing dot product between x and y

.. |function-math-exp| replace:: returns the e^x value of x

.. |function-math-exp2| replace:: returns the 2^x value of x

.. |function-math-fast_normalize| replace:: returns the fast approximation of normalized x, or nan if length(x) is 0

.. |function-math-floor| replace:: returns a float value representing the largest integer that is less than or equal to x

.. |function-math-floori| replace:: returns a integer value representing the largest integer that is less than or equal to x

.. |function-math-inv_distance| replace:: returns a non-negative value representing 1/distance between x and y

.. |function-math-inv_distance_sq| replace:: returns a non-negative value representing 1/squared distance between x and y

.. |function-math-inv_length| replace:: returns a non-negative value representing 1/magnitude of x

.. |function-math-inv_length_sq| replace:: returns a non-negative value representing 1/squared magnitude of x

.. |function-math-length| replace:: returns a non-negative value representing magnitude of x

.. |function-math-length_sq| replace:: returns a non-negative value representing squared magnitude of x

.. |function-math-lerp| replace:: returns vector or scalar representing a + (b - a) * t

.. |function-math-log| replace:: returns the natural logarithm of x

.. |function-math-log2| replace:: returns the logarithm base-2 of x

.. |function-math-mad| replace:: returns vector or scalar representing a * b + c

.. |function-math-max| replace:: returns the maximum of x and y

.. |function-math-min| replace:: returns the minimum of x and y

.. |function-math-normalize| replace:: returns normalized x, or nan if length(x) is 0

.. |function-math-pow| replace:: returns x raised to the power of y

.. |function-math-rcp| replace:: returns the 1/x

.. |function-math-rcp_est| replace:: returns the fast approximation 1/x

.. |function-math-reflect| replace:: see function-math-reflect.rst for details

.. |function-math-refract| replace:: see function-math-refract.rst for details

.. |function-math-roundi| replace:: returns a integer value representing the integer that is closest to x

.. |function-math-rsqrt| replace:: returns 1/sqrt(x)

.. |function-math-rsqrt_est| replace:: returns the fast approximation 1/sqrt(x)

.. |function-math-saturate| replace:: returns a clamped to [0..1] inclusive range x

.. |function-math-sin| replace:: returns the sine of x

.. |function-math-sincos| replace:: returns oth sine and cosine of x

.. |function-math-sqrt| replace:: returns the square root of x

.. |function-math-tan| replace:: returns the tangent of x

.. |function-math-trunci| replace:: returns a integer value representing the float without fraction part of x

.. |function-math-fract| replace:: returns a fraction part of x

.. |function-math-uint32_hash| replace:: returns hashed value of seed

.. |function-math-uint_noise_1D| replace:: returns noise value of position in the seeded sequence

.. |function-math-uint_noise_2D| replace:: returns noise value of position in the seeded sequence

.. |function-math-uint_noise_3D| replace:: returns noise value of position in the seeded sequence

.. |function-math--| replace:: returns -x


.. |variable-math-FLT_EPSILON| replace:: the difference between 1 and the smallest floating point number of type float that is greater than 1.

.. |variable-math-DBL_EPSILON| replace:: the difference between 1 and the smallest double precision floating point number of type double that is greater than 1.

.. |variable-math-PI| replace:: The ratio of a circle's circumference to its diameter. π

.. |function-math-!=| replace:: Compares x and y per component. Returns true if at least one component does not match.

.. |function-math-*| replace:: Multiplies x by y.

.. |function-math-==| replace:: Compares x and y per component. Returns false if at least one component does not match.

.. |function-math-float3x4| replace:: Returns empty matrix, where each component is 0.

.. |function-math-float4x4| replace:: Returns empty matrix, where each component is 0.

.. |function-math-float3x3| replace:: Returns empty matrix, where each component is 0.

.. |function-math-identity| replace:: Returns identity matrix, where diagonal is 1 and every other component is 0.

.. |function-math-identity4x4| replace:: Returns identity matrix, where diagonal is 1 and every other component is 0.

.. |function-math-identity3x4| replace:: Returns identity matrix, where diagonal is 1 and every other component is 0.

.. |function-math-identity3x3| replace:: Returns identity matrix, where diagonal is 1 and every other component is 0.

.. |function-math-inverse| replace:: Returns the inverse of the matrix x.

.. |function-math-rotate| replace:: Rotates vector y by 3x4 matrix x. Only 3x3 portion of x is multiplied by y.

.. |function-math-transpose| replace:: Transposes the specified input matrix x.

.. |any_annotation-math-float3x4| replace:: floating point matrix with 4 rows and 3 columns

.. |any_annotation-math-float4x4| replace:: floating point matrix with 4 rows and 4 columns

.. |any_annotation-math-float3x3| replace:: floating point matrix with 3 rows and 3 columns

.. |function-math-is_nan| replace:: Returns true if `x` is NaN (not a number)

.. |function-math-is_finite| replace:: Returns true if `x` is not a negative or positive infinity

.. |function-math-un_quat_from_unit_arc| replace:: Quaternion which represents rotation from `v0` to `v1`, both arguments need to be normalized

.. |function-math-un_quat_from_unit_vec_ang| replace:: Quaternion which represents rotation for `ang` radians around vector `v`. `v` needs to be normalized

.. |function-math-un_quat| replace:: Quaternion from the rotation part of the matrix

.. |function-math-quat_mul| replace:: Quaternion which is multiplication of `q1` and `q2`

.. |function-math-quat_mul_vec| replace:: Transform vector `v` by quaternion `q`

.. |function-math-quat_conjugate| replace:: Quaternion which is conjugate of `q`

.. |function-math-pack_float_to_byte| replace:: Packs float4 vector `v` to byte4 vector and returns it as uint. Each component is clamped to [0..255] range.

.. |function-math-unpack_byte_to_float| replace:: Unpacks byte4 vector to float4 vector.

.. |function-math-persp_forward| replace:: Perspective matrix, zn - 0, zf - 1

.. |function-math-persp_reverse| replace:: Perspective matrix, zn - 1, zf - 0

.. |function-math-look_at| replace:: Look-at matrix with the origin at `eye`, looking at `at`, with `up` as up direction.

.. |function-math-compose| replace:: Compose transformation out of translation, rotation and scale.

.. |function-math-decompose| replace:: Decompose transformation into translation, rotation and scale.

.. |function-math-orthonormal_inverse| replace:: Fast `inverse` for the orthonormal matrix.

.. |function-math-atan_est| replace:: Fast estimation for the `atan`.

.. |structure_annotation-math-float4x4| replace:: floating point matrix with 4 rows and 4 columns

.. |structure_annotation-math-float3x4| replace:: floating point matrix with 4 rows and 3 columns

.. |structure_annotation-math-float3x3| replace:: floating point matrix with 3 rows and 3 columns

.. |function-math-quat_from_unit_arc| replace:: Quaternion which represents rotation from `v0` to `v1`, both arguments need to be normalized

.. |function-math-quat_from_unit_vec_ang| replace:: Quaternion which represents rotation for `ang` radians around vector `v`. `v` needs to be normalized

.. |function-math-[]| replace:: Returns the component of the matrix `m` at the specified row.

.. |function-math-determinant| replace:: Returns the determinant of the matrix `m`.

.. |function-math-quat_from_euler| replace:: Construct quaternion from euler angles.

.. |function-math-euler_from_un_quat| replace:: Construct euler angles from quaternion.

