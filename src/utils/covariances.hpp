#ifndef RGBDSLAM_UTILS_COVARIANCES_HPP
#define RGBDSLAM_UTILS_COVARIANCES_HPP

#include "types.hpp"

namespace rgbd_slam {
    namespace utils {
        
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

    }   // utils
}       // rgbd_slam


#endif
