#ifndef RGBDSLAM_UTILS_COORDINATES_HPP
#define RGBDSLAM_UTILS_COORDINATES_HPP

#include "../types.hpp"

namespace rgbd_slam {
namespace utils {

    struct ScreenCoordinate2D;
    struct ScreenCoordinate;

    struct CameraCoordinate2D;
    struct CameraCoordinate;

    struct WorldCoordinate;

    struct PlaneCameraCoordinates;
    struct PlaneWorldCoordinates;

    /**
    * \brief Return true is a measurement is in the measurement range
    */
    bool is_depth_valid(const double depth);

    /**
     * \brief Contains a single of coordinate in screen space.
     * Screen space is defined as (x, y) in pixels
     */
    struct ScreenCoordinate2D : public vector2 {
        ScreenCoordinate2D() {};
        ScreenCoordinate2D(const vector2& coords) : vector2(coords) {};
        ScreenCoordinate2D(const double x, const double y) : vector2(x, y) {};

        /**
         * \brief Transform a screen point with a depth value to a 3D camera point
         *
         * \return A 3D point in camera coordinates
         */
        CameraCoordinate2D to_camera_coordinates() const;
    };

    /**
     * \brief Contains a single of coordinate in screen space.
     * Screen space is defined as (x, y) in pixels, and z in distance (millimeters)
     */
    struct ScreenCoordinate : public ScreenCoordinate2D {
        ScreenCoordinate() {};
        //ScreenCoordinate(const ScreenCoordinate& screenCoordinates) : ScreenCoordinate2D(screenCoordinates.x(), screenCoordinates.y()), _z(screenCoordinates.z()) {};
        ScreenCoordinate(const double x, const double y, const double z) : ScreenCoordinate2D(x, y), _z(z) {};

        /**
         * \brief Transform a screen point with a depth value to a 3D world point
         *
         * \param[in] cameraToWorld Matrix to transform local to world coordinates
         *
         * \return A 3D point in world coordinates
         */
        WorldCoordinate to_world_coordinates(const cameraToWorldMatrix& cameraToWorld) const;

        /**
         * \brief Transform a screen point with a depth value to a 3D camera point
         *
         * \return A 3D point in camera coordinates
         */
        CameraCoordinate to_camera_coordinates() const;

        double z() const { return _z; };

        vector3 base() const { return vector3(x(), y(), z()); };

    private:
        double _z;
    };


    /**
     * \brief Contains a single of coordinate in camera space.
     * Camera space is defined as (x, y), relative to the camera center
     */
    struct CameraCoordinate2D : public vector2 {
        CameraCoordinate2D() {};
        CameraCoordinate2D(const vector2& coords) : vector2(coords) {};
        CameraCoordinate2D(const double x, const double y) : vector2(x, y) {};

        /**
         * \brief Transform a point from camera to screen coordinate system
         *
         * \param[out] screenPoint The point screen coordinates, if the function returned true
         *
         * \return True if the screen position is valid
         */
        bool to_screen_coordinates(ScreenCoordinate2D& screenPoint) const;
    };


    /**
     * \brief Contains a single of coordinate in camera space.
     * Camera space is defined as (x, y, z) in distance (millimeters), relative to the camera center
     */
    struct CameraCoordinate : public CameraCoordinate2D {
        /**
         * \brief Scores a 3D coordinate in camera (x, y, depth). It can be projected to world space using a pose transformation
         */
        CameraCoordinate() {};
        CameraCoordinate(const vector3& coords) : CameraCoordinate2D(coords.x(), coords.y()), _z(coords.z()) {};
        CameraCoordinate(const vector4& homegenousCoordinates) : CameraCoordinate2D(
            homegenousCoordinates.x()/homegenousCoordinates[3],
            homegenousCoordinates.y()/homegenousCoordinates[3]
        ), _z(homegenousCoordinates.z()/homegenousCoordinates[3]) {};
        CameraCoordinate(const double x, const double y, const double z) : CameraCoordinate2D(x, y), _z(z) {};
        vector4 get_homogenous() const { return vector4(x(), y(), z(), 1);};

        /**
         * \brief Transform a camera point to a 3D world point
         *
         * \param[in] cameraToWorld Matrix to transform local to world coordinates
         *
         * \return A 3D point in world coordinates
         */
        WorldCoordinate to_world_coordinates(const cameraToWorldMatrix& cameraToWorld) const;

        /**
         * \brief Transform a point from camera to screen coordinate system
         *
         * \param[out] screenPoint The point screen coordinates, if the function returned true
         *
         * \return True if the screen position is valid
         */
        bool to_screen_coordinates(ScreenCoordinate& screenPoint) const;
        bool to_screen_coordinates(ScreenCoordinate2D& screenPoint) const;

        double z() const { return _z; };

        vector3 base() const { return vector3(x(), y(), z()); };

    private:
        double _z;
    };


    /**
     * \brief Contains a single of coordinate in world space.
     * World space is defined as (x, y, z) in distance (millimeters), relative to the world center
     */
    struct WorldCoordinate : public vector3 {
        /**
         * \brief Scores a 3D coordinate in world space (x, y, depth).
         */
        WorldCoordinate() {};
        WorldCoordinate(const vector3& coords) : vector3(coords) {};
        WorldCoordinate(const double x, const double y, const double z) : vector3(x, y, z) {};

        /**
         * \brief Transform a point from world to screen coordinate system
         *
         * \param[in] worldToCamera Matrix to transform the world to a local coordinate system
         * \param[out] screenPoint The point screen coordinates, if the function returned true
         *
         * \return True if the screen position is valid
         */
        bool to_screen_coordinates(const worldToCameraMatrix& worldToCamera, ScreenCoordinate& screenPoint) const;
        bool to_screen_coordinates(const worldToCameraMatrix& worldToCamera, ScreenCoordinate2D& screenPoint) const;

        /**
         * \brief Transform a vector in world space to a vector in camera space
         *
         * \param[in] worldToCamera Matrix to transform the world to a local coordinate system
         *
         * \return The input vector transformed to camera space
         */
        CameraCoordinate to_camera_coordinates(const worldToCameraMatrix& worldToCamera) const;

        /**
         * \brief Compute a signed 2D distance between this world point and a screen point, by retroprojecting the world point to screen space
         * \param[in] screenPoint A point in screen space. Only the x and y components will be used
         * \param[in] worldToCamera A transformation matrix to convert from world to camera space
         * \return a 2D signed distance in camera space (pixels)
         */
        vector2 get_signed_distance_2D(const ScreenCoordinate2D& screenPoint, const worldToCameraMatrix& worldToCamera) const;
        /**
         * \brief Compute a distance between this world point and a screen point, by retroprojecting the world point to screen space.
         * \param[in] screenPoint A point in screen space. Only the x and y components will be used
         * \param[in] worldToCamera A transformation matrix to convert from world to camera space
         * \return an unsigned distance in camera space (pixels)
         */
        double get_distance(const ScreenCoordinate2D& screenPoint, const worldToCameraMatrix& worldToCamera) const;
        /**
         * \brief Compute a signed distance between a world point and a 3D point in screen space, by projecting the screen point to world space
         * \param[in] screenPoint A 3D point in screen space
         * \param[in] cameraToWorld A matrix to convert from camera to world space
         * \return The 3D signed distance in world space
         */
        vector3 get_signed_distance(const ScreenCoordinate& screenPoint, const cameraToWorldMatrix& cameraToWorld) const;
        /**
         * \brief Compute a distance between a world point and a 3D point in screen space, by projecting the screen point to world space
         * \param[in] screenPoint A 3D point in screen space
         * \param[in] cameraToWorld A matrix to convert from camera to world space
         * \return The unsigned distance in world space
         */
        double get_distance(const ScreenCoordinate& screenPoint, const cameraToWorldMatrix& cameraToWorld) const;
        /**
         * \brief Compute a signed distance with another world point
         */
        vector3 get_signed_distance(const WorldCoordinate& worldPoint) const { return  this->base() - worldPoint; };
        double get_distance(const WorldCoordinate& worldPoint) const { return get_signed_distance(worldPoint).lpNorm<1>(); };
    };


    struct PlaneCameraCoordinates : vector4 {
        PlaneCameraCoordinates() {};
        PlaneCameraCoordinates(const vector4& plane) : vector4(plane) {};
        PlaneCameraCoordinates(const double x, const double y, const double z, const double d) : vector4(x, y, z, d) {};

        PlaneWorldCoordinates to_world_coordinates(const planeCameraToWorldMatrix& cameraToWorld) const;
    };

    struct PlaneWorldCoordinates : vector4 {
        PlaneWorldCoordinates() {};
        PlaneWorldCoordinates(const vector4& plane) : vector4(plane) {};
        PlaneWorldCoordinates(const double x, const double y, const double z, const double d) : vector4(x, y, z, d) {};

        PlaneCameraCoordinates to_camera_coordinates(const planeWorldToCameraMatrix& worldToCamera) const;
        
        
        /**
         * \brief Compute a distance between two planes, by retroprojecting a world plane to camera space
         * \param[in] cameraPlane A plane in camera coordinates
         * \param[in] worldToCamera A transformation matrix to convert from world to camera space
         *
         * \return A 3D vector of the error between the two planes. The x and y are angle distances, the z is in millimeters
         */
        vector4 get_signed_distance(const PlaneCameraCoordinates& cameraPlane, const planeWorldToCameraMatrix& worldToCamera) const;
        /**
         * \brief Compute a distance between two planes, by retroprojecting a world plane to camera space. Result is reduced to two angles and a distance
         * \param[in] cameraPlane A plane in camera coordinates
         * \param[in] worldToCamera A transformation matrix to convert from world to camera space
         *
         * \return A 3D vector of the error between the two planes. The x and y are angle distances, the z is in millimeters
         */
        vector3 get_reduced_signed_distance(const PlaneCameraCoordinates& cameraPlane, const planeWorldToCameraMatrix& worldToCamera) const;
    };

}
}

#endif