#ifndef TYPE_DEFINITION_HPP
#define TYPE_DEFINITION_HPP


#include <Eigen/StdVector>

namespace poseEstimation {
    /*
     *        Declare the most common types used in this program
     */

    typedef Eigen::Vector2d vector2;
    typedef Eigen::Matrix<double, 3, 1> vector3;
    typedef Eigen::Vector4d vector4;
    typedef Eigen::Matrix3d matrix33;
    typedef Eigen::Matrix<double, 3, 4> matrix34;
    typedef Eigen::Matrix4d matrix44;
    typedef Eigen::Quaternion<double> quaternion;

    typedef std::vector<vector2, Eigen::aligned_allocator<vector2>> vector2_array;
    typedef std::vector<vector3, Eigen::aligned_allocator<vector3>> vector3_array;
    typedef std::vector<vector4, Eigen::aligned_allocator<vector4>> vector4_array;
    typedef std::vector<matrix33, Eigen::aligned_allocator<matrix33>> matrix33_array;
    typedef std::vector<matrix34, Eigen::aligned_allocator<matrix34>> matrix34_array;
    typedef std::vector<matrix44, Eigen::aligned_allocator<matrix44>> matrix44_array;
    typedef std::vector<quaternion, Eigen::aligned_allocator<quaternion>> quaternion_array;

}

#endif