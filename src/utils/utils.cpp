#include "utils.hpp"

#include "parameters.hpp"
#include <cmath>
#include <filesystem>

namespace rgbd_slam {
    namespace utils {

        void log(std::string_view message, const std::source_location& location)
        {
            std::cout << "[INF] "
                << std::filesystem::path(location.file_name()).filename().string() << "("
                << location.line() << ":" << location.column() <<  ") "
                << location.function_name() << " | "
                << message << std::endl;
        }
        void log_error(std::string_view message, const std::source_location& location)
        {
            std::cerr << "[ERR] "
                << std::filesystem::path(location.file_name()).filename().string() << "("
                << location.line() << ":" << location.column() <<  ") "
                << location.function_name() << " | "
                << message << std::endl;
        }

        const vector3 screen_to_world_coordinates(const double screenX, const double screenY, const double measuredZ, const matrix34& cameraToWorldMatrix) 
        {
            const double depthMillimeters = measuredZ;
            const double x = (screenX - Parameters::get_camera_1_center_x()) * depthMillimeters / Parameters::get_camera_1_focal_x();
            const double y = (screenY - Parameters::get_camera_1_center_y()) * depthMillimeters / Parameters::get_camera_1_focal_y();

            vector4 worldPoint;
            worldPoint << x, y, depthMillimeters, 1.0;
            return cameraToWorldMatrix * worldPoint;
        }

        const vector2 world_to_screen_coordinates(const vector3& position3D, const matrix34& worldToCameraMatrix)
        {
            vector4 ptH;
            ptH << position3D, 1.0;
            const vector3& point3D = (worldToCameraMatrix * ptH); 

            if (point3D.z() == 0) {
                return vector2(0.0, 0.0);
            }

            const double inverseDepth  = 1.0 / point3D.z();
            const double screenX = Parameters::get_camera_1_focal_x() * point3D.x() * inverseDepth + Parameters::get_camera_1_center_x();
            const double screenY = Parameters::get_camera_1_focal_y() * point3D.y() * inverseDepth + Parameters::get_camera_1_center_y();

            return vector2(screenX, screenY);
        }

        const matrix34 compute_camera_to_world_transform(const quaternion& rotation, const vector3& position)
        {
            matrix34 cameraToWorldMatrix;
            cameraToWorldMatrix << rotation.toRotationMatrix(), (position);
            return cameraToWorldMatrix;
        }

        const matrix34 compute_world_to_camera_transform(const quaternion& rotation, const vector3& position)
        {
            const matrix33& worldToCamRotMtrx = (rotation.toRotationMatrix()).transpose();
            const vector3& worldToCamTranslation = (-worldToCamRotMtrx) * (position);
            matrix34 worldToCamMtrx;
            worldToCamMtrx << worldToCamRotMtrx, worldToCamTranslation;
            return worldToCamMtrx;
        }

        const matrix33 get_world_point_covariance(const vector2& screenPoint, const double depth, const matrix33& screenPointError)
        {
            const double cameraFX = Parameters::get_camera_1_focal_x();
            const double cameraFY = Parameters::get_camera_1_focal_y();
            const double cameraCX = Parameters::get_camera_1_center_x();
            const double cameraCY = Parameters::get_camera_1_center_y();

            const matrix33 jacobian {
                {depth / cameraFX, 0.0, (screenPoint.x() - cameraCX) / cameraFX },
                    {0.0, depth / cameraFY, (screenPoint.y() - cameraCY) / cameraFY },
                    {0.0, 0.0, 1.0}
            };

            return jacobian * screenPointError * jacobian.transpose();
        }


        const quaternion get_quaternion_from_euler_angles(const EulerAngles& eulerAngles)
        {
            const double cy = cos(eulerAngles.yaw * 0.5);
            const double sy = sin(eulerAngles.yaw * 0.5);
            const double cp = cos(eulerAngles.pitch * 0.5);
            const double sp = sin(eulerAngles.pitch * 0.5);
            const double cr = cos(eulerAngles.roll * 0.5);
            const double sr = sin(eulerAngles.roll * 0.5);

            const quaternion quat(
                    cr * cp * cy + sr * sp * sy,
                    sr * cp * cy - cr * sp * sy,
                    cr * sp * cy + sr * cp * sy,
                    cr * cp * sy - sr * sp * cy
                    );
            return quat;
        }

        const EulerAngles get_euler_angles_from_quaternion(const quaternion& quat)
        {
            EulerAngles eulerAngles;
            eulerAngles.yaw = std::atan2(
                    2 * (quat.w() * quat.x() + quat.y() * quat.z()),
                    1 - 2 * (quat.x() * quat.x() + quat.y() * quat.y())
                    );

            const double sinp = 2 * (quat.w() * quat.y() - quat.z() * quat.x());
            if (std::abs(sinp) >= 1)
                eulerAngles.pitch = std::copysign(M_PI / 2, sinp);
            else
                eulerAngles.pitch = std::asin(sinp);

            eulerAngles.roll = std::atan2(
                    2 * (quat.w() * quat.z() + quat.x() * quat.y()),
                    1 - 2 * (quat.y() * quat.y() + quat.z() * quat.z())
                    );

            return eulerAngles;
        }

    }
}
