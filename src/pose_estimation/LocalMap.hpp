#ifndef LOCAL_MAP_HPP
#define LOCAL_MAP_HPP

#include "Pose.hpp"
#include "Parameters.hpp"

#include <opencv2/opencv.hpp>
#include <vector>

namespace poseEstimation {
    class Image_Features_Struct;
    class Image_Features_Handler;

    class Local_Map {

        public:
            /**
             * \brief Keeps a reference to the featuresHandler object
             */
            Local_Map(const Parameters& voparams,
                    Image_Features_Handler* featureHandler);
            ~Local_Map();
            void reset();

            /**
             * \brief Triangulate new map points from features that were not matched/tracked
             *
             * \param[in] camPose Position of camera
             * \param[in] features
             * \param[in] dontStage Dont store staged points
             */
            void update_with_new_triangulation(const Pose& camPose, Image_Features_Struct& features, bool dontStage = false);


            /**
             * \brief Mark untracked points as untracked in map
             *
             * \param[in, out] features
             */
            void clean_untracked_points(Image_Features_Struct& features);

            /**
             * \brief Update the stagePoints vector 
             *
             * \param[in] camPose
             * \param[in, out] features Features to treat, can be marked as matched
             */
            void update_staged_map_points(const Pose& camPose, Image_Features_Struct& features);


        public: //getters
            unsigned int get_map_size() const { return _mapPoints.size(); }
            unsigned int get_staged_points_count() const { return _stagedPoints.size(); }

            /**
             * \brief Compute point matches from current pose in the local map
             *
             * \param[in] camPose Position of the observer
             * \param[in, out] features Current image features
             * \param[out] outMapPoints Matched points
             * \param[out] outMatchesLeft Unmatched points (outliers)
             *
             * \return matched point count
             */
            unsigned int find_matches(const Pose &camPose, Image_Features_Struct& features,
                    vector3_array& outMapPoints, std::vector<int>& outMatchesLeft);

        private:
            struct mapPoint
            {
                EIGEN_MAKE_ALIGNED_OPERATOR_NEW
                    cv::Mat descriptor;     // descriptor for the keypoint used initially to triangulate this map point
                vector3 position;       // world position of this point
                unsigned int counter;   // depending on context, if the point is staged then this is the number of frames it has been tracked while staged. If it is a map point, then this is the number of times it failed tracking.
                unsigned int age;       // number of frames this map point was successfully tracked and thus used in pose estimation
                int match_idx;
            };
            typedef std::vector<mapPoint, Eigen::aligned_allocator<mapPoint>> mapPointArray;


            Parameters _voParams;
            Image_Features_Handler *_featuresHandler;

            mapPointArray _mapPoints;
            mapPointArray _stagedPoints;


        protected:

            //void triangulate(const Pose &camPose, Image_Features_Struct& features, Image_Features_Struct& featuresRight, mapPointArray& outPoints);

            /**
             *
             * \param[in] camPose Observer position
             * \param[in] features Detected features in RGB image
             * \param[out] outPoints 
             */
            void triangulate_rgbd(const Pose& camPose, Image_Features_Struct& features, mapPointArray& outPoints);

    };

}

#endif
