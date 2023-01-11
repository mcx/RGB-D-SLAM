#ifndef RGBDSLAM_MAPMANAGEMENT_MAPPOINT_HPP
#define RGBDSLAM_MAPMANAGEMENT_MAPPOINT_HPP

#include <opencv2/opencv.hpp>

#include "../utils/coordinates.hpp"
#include "../tracking/kalman_filter.hpp"
#include "../features/keypoints/keypoint_handler.hpp"
#include "feature_map.hpp"
#include "parameters.hpp"

namespace rgbd_slam {
    namespace map_management {

        const size_t INVALID_POINT_UNIQ_ID = 0; // This id indicates an invalid unique id for a map point

        struct Point
        {
            // world coordinates
            utils::WorldCoordinate _coordinates;
            // 3D descriptor (ORB)
            cv::Mat _descriptor;
            // position covariance
            matrix33 _covariance;

            Point(const utils::WorldCoordinate& coordinates, const matrix33& covariance, const cv::Mat& descriptor) :
                _coordinates(coordinates), 
                _descriptor(descriptor),
                _covariance(covariance)
            {
                build_kalman_filter();

                assert(not _descriptor.empty() and _descriptor.cols > 0);
                assert(not std::isnan(_coordinates.x()) and not std::isnan(_coordinates.y()) and not std::isnan(_coordinates.z()));
            };

            /**
             * \brief update the current point by tracking with a kalman filter. Will update the point position & covariance
             * \return The distance between the new position ans the previous one
             */
            double track_point(const utils::WorldCoordinate& newPointCoordinates, const matrix33& newPointCovariance)
            {
                assert(_kalmanFilter != nullptr);

                const std::pair<vector3, matrix33>& res = _kalmanFilter->get_new_state(_coordinates, _covariance, newPointCoordinates, newPointCovariance);

                const double score = (_coordinates - res.first).norm();

                _coordinates = res.first;
                _covariance = res.second;
                assert(not std::isnan(_coordinates.x()) and not std::isnan(_coordinates.y()) and not std::isnan(_coordinates.z()));
                return score;
            }

            private:
            /**
             * \brief Build the inputs caracteristics of the kalman filter
             */
            static void build_kalman_filter()
            {
                if (_kalmanFilter == nullptr)
                {
                    // gain 10mm of uncertainty at each iteration
                    const double pointProcessNoise = 0;   // TODO set in parameters
                    const size_t stateDimension = 3;        //x, y, z
                    const size_t measurementDimension = 3;  //x, y, z

                    matrixd systemDynamics(stateDimension, stateDimension); // System dynamics matrix
                    matrixd outputMatrix(measurementDimension, stateDimension); // Output matrix
                    matrixd processNoiseCovariance(stateDimension, stateDimension); // Process noise covariance

                    // Points are not supposed to move, so no dynamics
                    systemDynamics.setIdentity();
                    // we need all positions
                    outputMatrix.setIdentity();

                    processNoiseCovariance.setIdentity();
                    processNoiseCovariance *= pointProcessNoise;

                    _kalmanFilter = new tracking::SharedKalmanFilter(systemDynamics, outputMatrix, processNoiseCovariance);
                }
            }
            // shared kalman filter, between all points
            inline static tracking::SharedKalmanFilter* _kalmanFilter = nullptr;
        };

        typedef features::keypoints::Keypoint_Handler DetectedKeypointsObject;
        typedef features::keypoints::DetectedKeyPoint DetectedPointType;
        typedef matches_containers::PointMatch PointMatchType;
        typedef features::keypoints::KeypointsWithIdStruct TrackedPointsObject;


        class MapPoint
            : public Point, public IMapFeature<DetectedKeypointsObject, DetectedPointType, PointMatchType, TrackedPointsObject>
        {
            public:
            MapPoint(const utils::WorldCoordinate& coordinates, const matrix33& covariance, const cv::Mat& descriptor) :
                Point(coordinates, covariance, descriptor),
                IMapFeature<DetectedKeypointsObject, DetectedPointType, PointMatchType, TrackedPointsObject>()
            {
                assert(_id > 0);
            }

            MapPoint(const utils::WorldCoordinate& coordinates, const matrix33& covariance, const cv::Mat& descriptor, const size_t id) :
                Point(coordinates, covariance, descriptor),
                IMapFeature<DetectedKeypointsObject, DetectedPointType, PointMatchType, TrackedPointsObject>(id)
            {
                assert(_id > 0);
            }

            virtual int find_match(const DetectedKeypointsObject& detectedFeatures, const worldToCameraMatrix& worldToCamera, const vectorb& isDetectedFeatureMatched, std::list<PointMatchType>& matches, const bool shouldAddToMatches = true, const bool useAdvancedSearch = false) const override 
            {
                static const double searchSpaceRadius = Parameters::get_search_matches_distance();
                static const double advancedSearchSpaceRadius = Parameters::get_search_matches_distance() * 2;
                const double searchRadius = useAdvancedSearch ? advancedSearchSpaceRadius : searchSpaceRadius;

                // try to match with tracking
                const int invalidfeatureIndex = features::keypoints::INVALID_MATCH_INDEX;
                int matchIndex = detectedFeatures.get_tracking_match_index(_id, isDetectedFeatureMatched);
                if (matchIndex == invalidfeatureIndex)
                {
                    // No match: try to find match in a window around the point
                    utils::ScreenCoordinate2D projectedMapPoint;
                    const bool isScreenCoordinatesValid = _coordinates.to_screen_coordinates(worldToCamera, projectedMapPoint);
                    if (isScreenCoordinatesValid)
                    {
                        matchIndex = detectedFeatures.get_match_index(projectedMapPoint, _descriptor, isDetectedFeatureMatched, searchRadius);
                    }
                }

                if (matchIndex == invalidfeatureIndex) {
                    //unmatched point
                    return UNMATCHED_FEATURE_INDEX;
                }

                assert(matchIndex >= 0);
                assert(static_cast<Eigen::Index>(matchIndex) < isDetectedFeatureMatched.size());
                if (isDetectedFeatureMatched[matchIndex])
                {
                    //point was already matched
                    outputs::log_error("The requested point unique index is already matched");
                }

                if (shouldAddToMatches)
                {
                    const utils::ScreenCoordinate& matchedScreenpoint = detectedFeatures.get_keypoint(matchIndex);
                    if (utils::is_depth_valid(matchedScreenpoint.z()) ) 
                    {
                        // 3D point
                        const rgbd_slam::screenCoordinateCovariance& screenCovariance = utils::get_screen_point_covariance(_coordinates, _covariance);
                        //consider only the diagonal part of the matrix: it is the 2D variance en x/y in screen space
                        const vector2& screenPointCovariance(screenCovariance.diagonal().head(2));
                        matches.emplace_back(PointMatchType(matchedScreenpoint, _coordinates, screenPointCovariance, _id));
                    }
                    else {
                        // 2D point
                        const rgbd_slam::screenCoordinateCovariance& screenCovariance = utils::get_screen_point_covariance(_coordinates, _covariance);
                        //consider only the diagonal part of the matrix: it is the 2D variance en x/y in screen space
                        const vector2& screenPointCovariance(screenCovariance.diagonal().head(2));
                        matches.emplace_back(PointMatchType(matchedScreenpoint, _coordinates, screenPointCovariance, _id));
                    }
                }
                return matchIndex;
            }

            virtual bool add_to_tracked(const worldToCameraMatrix& worldToCamera, TrackedPointsObject& trackedFeatures, const uint dropChance = 1000) const override
            {
                const bool shouldNotDropPoint = utils::Random::get_random_uint(dropChance) != 0;

                assert(not std::isnan(_coordinates.x()) and not std::isnan(_coordinates.y()) and not std::isnan(_coordinates.z()));
                if (shouldNotDropPoint)
                {
                    utils::ScreenCoordinate2D screenCoordinates;
                    if (_coordinates.to_screen_coordinates(worldToCamera, screenCoordinates))
                    {
                        // use previously known screen coordinates
                        trackedFeatures.add(
                            _id,
                            screenCoordinates.x(),
                            screenCoordinates.y()
                        );

                        return true;
                    }
                }
                // point was not added
                return false;
            }

            virtual void draw(const worldToCameraMatrix& worldToCamMatrix, cv::Mat& debugImage, const cv::Scalar& color) const override
            {
                utils::ScreenCoordinate2D screenPoint;
                const bool isCoordinatesValid = _coordinates.to_screen_coordinates(worldToCamMatrix, screenPoint);

                if (isCoordinatesValid)
                {
                    cv::circle(debugImage, cv::Point(static_cast<int>(screenPoint.x()), static_cast<int>(screenPoint.y())), 3, color, -1);
                }
            }

            virtual bool is_visible(const worldToCameraMatrix& worldToCamMatrix) const override
            {
                static const uint screenSizeX = Parameters::get_camera_1_size_x(); 
                static const uint screenSizeY = Parameters::get_camera_1_size_y();
 
                utils::ScreenCoordinate projectedScreenCoordinates;
                const bool isProjected = _coordinates.to_screen_coordinates(worldToCamMatrix, projectedScreenCoordinates);
                if (isProjected)
                {
                    return 
                        // in screen space
                        projectedScreenCoordinates.x() >= 0 and projectedScreenCoordinates.x() <= screenSizeX and 
                        projectedScreenCoordinates.y() >= 0 and projectedScreenCoordinates.y() <= screenSizeY and
                        // in front of the camera
                        projectedScreenCoordinates.z() >= 0;
                }
                return false;
            }

            protected:
            virtual bool update_with_match(const DetectedPointType& matchedFeature, const matrix33& poseCovariance, const cameraToWorldMatrix& cameraToWorld) override
            {
                assert(_matchIndex >= 0);

                const utils::ScreenCoordinate& matchedScreenPoint = matchedFeature._coordinates;
                if(utils::is_depth_valid(matchedScreenPoint.z()))
                {
                    // transform screen point to world point
                    const utils::WorldCoordinate& worldPointCoordinates = matchedScreenPoint.to_world_coordinates(cameraToWorld);
                    // get a measure of the estimated variance of the new world point
                    const cameraCoordinateCovariance& cameraPointCovariance = utils::get_camera_point_covariance(matchedScreenPoint);

                    // update this map point errors & position
                    track_point(worldPointCoordinates, cameraPointCovariance.base() + poseCovariance);

                    // If a new descriptor is available, update it
                    const cv::Mat& descriptor = matchedFeature._descriptor;
                    if (not descriptor.empty())
                        _descriptor = descriptor;

                    return true;
                }
                else
                {
                    // TODO: Point is 2D, handle separatly
                }
                return false;
            }

            virtual void update_no_match() override 
            {
                // do nothing
            }
        };

        /**
         * \brief Candidate for a map point
         */
        class StagedMapPoint
            : public virtual MapPoint, public virtual IStagedMapFeature<DetectedPointType>
        {
            public:
                StagedMapPoint(const matrix33& poseCovariance, const cameraToWorldMatrix& cameraToWorld, const DetectedPointType& detectedFeature) :
                    MapPoint(
                        detectedFeature._coordinates.to_world_coordinates(cameraToWorld),
                        utils::get_camera_point_covariance(detectedFeature._coordinates) + poseCovariance,
                        detectedFeature._descriptor
                    )
                {
                }

                virtual bool should_remove_from_staged() const override
                {
                    return get_confidence() <= 0;
                }

                virtual bool should_add_to_local_map() const override
                {
                    const static double minimumConfidenceForLocalMap = Parameters::get_minimum_confidence_for_local_map();
                    return (get_confidence() > minimumConfidenceForLocalMap);
                }

            protected:
                double get_confidence() const 
                {
                    const static double stagedPointconfidence = static_cast<double>(Parameters::get_point_staged_age_confidence());
                    const double confidence = static_cast<double>(_successivMatchedCount) / stagedPointconfidence;
                    return std::clamp(confidence, -1.0, 1.0);
                }
        };


        /**
         * \brief A map point structure, containing all the necessary informations to identify a map point in local map
         */
        class LocalMapPoint 
            : public MapPoint, public ILocalMapFeature<StagedMapPoint>
        {
            public:
                LocalMapPoint(const StagedMapPoint& stagedPoint) : 
                    MapPoint(stagedPoint._coordinates, stagedPoint._covariance, stagedPoint._descriptor, stagedPoint._id)
                {
                    // new map point, new color
                    set_color();

                    _matchIndex = stagedPoint._matchIndex;
                    _successivMatchedCount = stagedPoint._successivMatchedCount;
                }

                virtual bool is_lost() const override
                {
                    const static uint maximumUnmatchBeforeRemoval = Parameters::get_maximum_unmatched_before_removal();
                    return (_failedTrackingCount > maximumUnmatchBeforeRemoval);
                }
        };

    } /* map_management */
} /* rgbd_slam */

#endif
