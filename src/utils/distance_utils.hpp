#ifndef RGBDSLAM_UTILS_DISTANCE_UTILS_HPP
#define RGBDSLAM_UTILS_DISTANCE_UTILS_HPP

#include "coordinates.hpp"

namespace rgbd_slam {
    namespace utils {

        /**
         * \brief Compute a signed distance between two points
         */
        Eigen::VectorXd get_signed_distance(const Eigen::VectorXd& pointA, const Eigen::VectorXd& pointB);

        /**
         * \brief Compute a signed 2D distance between a world point and a camera point, by retroprojecting the world point to camera space.
         * \param[in] worldPoint A 3D point in world space
         * \param[in] cameraPoint A point in camera space. Only the x and y components will be used
         * \param[in] worldToCamera A transformation matrix to convert from world to camera space
         * \return a 2D signed distance in camera space (pixels)
         */
        vector2 get_3D_to_2D_distance_2D(const WorldCoordinate& worldPoint, const ScreenCoordinate& cameraPoint, const worldToCameraMatrix& worldToCamera);

        /**
         * \brief Compute a distance between a world point and a camera point, by retroprojecting the world point to camera space.
         * \param[in] worldPoint A 3D point in world space
         * \param[in] cameraPoint A point in camera space. Only the x and y components will be used
         * \param[in] worldToCamera A transformation matrix to convert from world to camera space
         * \return an unsigned distance in camera space (pixels)
         */
        double get_3D_to_2D_distance(const WorldCoordinate& worldPoint, const ScreenCoordinate& cameraPoint, const worldToCameraMatrix& worldToCamera);

        /**
         * \brief Compute a signed distance between a world point and a 3D point in camera space, by projecting the camera point to world space
         * \param[in] worldPoint A 3D point in world space
         * \param[in] cameraPoint A 3D point in camera space
         * \param[in] cameraToWorld A matrix to convert from camera to world space
         * \return The 3D signed distance in world space
         */
        vector3 get_3D_to_3D_distance_3D(const WorldCoordinate& worldPoint, const ScreenCoordinate& cameraPoint, const cameraToWorldMatrix& cameraToWorld);

        /**
         * \brief Compute a distance between a world point and a 3D point in camera space, by projecting the camera point to world space
         * \param[in] worldPoint A 3D point in world space
         * \param[in] cameraPoint A 3D point in camera space
         * \param[in] cameraToWorld A matrix to convert from camera to world space
         * \return The unsigned distance in world space
         */
        double get_3D_to_3D_distance(const WorldCoordinate& worldPoint, const ScreenCoordinate& cameraPoint, const cameraToWorldMatrix& cameraToWorld);

    }   // utils
}       // rgbd_slam


#endif
