#ifndef RGBDSLAM_UTILS_UTILS_HPP
#define RGBDSLAM_UTILS_UTILS_HPP

#include "types.hpp"

#include <string_view>
#include <source_location>


namespace rgbd_slam {
    namespace utils {

        /**
         * Log an error line
         */
        void log(std::string_view message, const std::source_location& location = std::source_location::current());
        void log_error(std::string_view message, const std::source_location& location = std::source_location::current());

        /*
         * \brief Transform a screen point with a depth value to a 3D point
         *
         * \param[in] screenX X coordinates of the 2D point (double because we can have sub pixel accuracy)
         * \param[in] screenY Y coordinates of the 2D point (double because we can have sub pixel accuracy)
         * \param[in] measuredZ Measured z depth of the point, in millimeters
         * \param[in] cameraToWorldMatrix Matrix to transform local to world coordinates
         *
         * \return A 3D point in frame coordinates
         */
        const vector3 screen_to_world_coordinates(const double screenX, const double screenY, const double measuredZ, const matrix44& cameraToWorldMatrix);

        /**
         * \brief Transform a vector in screen space to a vector in world space
         *
         * \param[in] vector4d A vector in screen space
         * \param[in] cameraToWorldMatrix Matrix to transform local to world coordinates
         *
         * \return A vector in world space
         */
        const vector4 screen_to_world_coordinates(const vector4& vector4d, const matrix44& cameraToWorldMatrix);


        /**
         * \brief Transform a point from world to screen coordinate system
         *
         * \param[in] position3D Coordinates of the detected point (world coordinates)
         * \param[in] worldToScreenMatrix Matrix to transform the world to a local coordinate system
         * \param[out] screenCoordinates The point screen coordinates, if the function returned true
         *
         * \return True if the screen position is valid
         */
        bool world_to_screen_coordinates(const vector3& position3D, const matrix44& worldToScreenMatrix, vector2& screenCoordinates);

        /**
         * \brief Transform a vector in world space to a vector in screen space
         *
         * \param[in] worldVector4 A vector in screen space
         * \param[in] worldToScreenMatrix Matrix to transform the world to a local coordinate system
         *
         * \return The input vector transformed to screen space
         */
        const vector4 world_to_screen_coordinates(const vector4& worldVector4, const matrix44& worldToScreenMatrix);

        /**
         * \brief Given a camera pose, returns a transformation matrix to convert a world point (xyz) to camera point (uvd)
         */
        const matrix44 compute_world_to_camera_transform(const quaternion& rotation, const vector3& position);

        const matrix44 compute_world_to_camera_transform(const matrix44& cameraToWorldMatrix);

        /**
         * \brief Given a camera pose, returns a transformation matrix to convert a camera point (uvd) to world point (xyz)
         */
        const matrix44 compute_camera_to_world_transform(const quaternion& rotation, const vector3& position);

        /**
         * \brief Compute the associated Gaussian error of a screen point when it will be transformed to world point
         *
         * \param[in] screenPoint The 2D point in screen coordinates
         * \param[in] depth The depth associated with this screen point
         * \param[in] screenPointCovariance The covariance matrix associated with a point in screen space
         *
         * \return the covariance of the 3D world point
         */
        const matrix33 get_world_point_covariance(const vector2& screenPoint, const double depth, const matrix33& screenPointCovariance);

        /**
         * \brief Compute a quaternion from the given euler angles, in radians
         */
        const quaternion get_quaternion_from_euler_angles(const EulerAngles& eulerAngles);

        /**
         * \brief Compute euler angles from a given quaternion
         */
        const EulerAngles get_euler_angles_from_quaternion(const quaternion& quat);

        /**
         * \brief Compute a rotation matrix from a euler angle container
         */
        const matrix33 get_rotation_matrix_from_euler_angles(const EulerAngles& eulerAngles);

    }
}




#endif
