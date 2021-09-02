#ifndef SLAM_LOCAL_MAP_HPP
#define SLAM_LOCAL_MAP_HPP 

#include <list>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>

#include "types.hpp"
#include "map_point.hpp"
#include "KeyPointDetection.hpp"
#include "Pose.hpp"


namespace rgbd_slam {
    namespace map_management {

        /**
         * \brief Maintain a local map around the camera. Can return matched features, and update the global map when features are estimated to be reliable. For now we dont have a global map
         */
        class Local_Map {
            public:
                Local_Map();

                /**
                 * \brief Compute the point feature matches between the local map and a given set of points. Update the staged point list matched points
                 */
                match_point_container find_matches(const utils::Keypoint_Handler& detectedKeypoint); 

                /**
                 * \brief Update the local and global map 
                 */
                void update(const poseEstimation::Pose optimizedPose, const utils::Keypoint_Handler& keypointObject);


                /**
                 * \brief Hard clean the local map
                 */
                void reset();


                /**
                 * \brief Compute a debug image
                 *
                 * \param[in] camPose Pose of the camera in world coordinates
                 * \param[in, out] debugImage Output image
                 */

                void get_debug_image(const poseEstimation::Pose& camPose, cv::Mat& debugImage) const;

            protected:
                /**
                 * \brief Add previously uncertain features to the local map
                 */
                void update_staged(const poseEstimation::Pose optimizedPose, const utils::Keypoint_Handler& keypointObject);

                /**
                 * \brief Clean the local map so it stays local, and update the global map with the good features
                 */
                void update_local_to_global();

            private:
                unsigned int _currentIndex;

                //local point map
                typedef std::list<utils::Map_Point> point_map_container;
                point_map_container _localMap;

                std::vector<bool> _unmatched;

                //local primitive map


        };

    }
}

#endif