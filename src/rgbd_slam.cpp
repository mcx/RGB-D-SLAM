#include "rgbd_slam.hpp"
#include "camera_transformation.hpp"
#include "covariances.hpp"
#include "outputs/logger.hpp"
#include "parameters.hpp"
#include "pose_optimization/pose_optimization.hpp"
#include "matches_containers.hpp"
#include "utils/random.hpp"
#include <future>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>

namespace rgbd_slam {

RGBD_SLAM::RGBD_SLAM(const utils::Pose& startPose, const uint imageWidth, const uint imageHeight) :
    _width(imageWidth),
    _height(imageHeight),
    _isTrackingLost(true),
    _failedTrackingCount(0),
    _isFirstTrackingCall(true),

    _totalFrameTreated(0),
    _meanDepthMapTreatmentDuration(0.0),
    _meanPoseOptimizationDuration(0.0),
    _meanPrimitiveTreatmentDuration(0.0),
    _meanLineTreatmentDuration(0.0),
    _meanFindMatchTime(0.0),
    _meanPoseOptimizationFromFeatures(0.0),
    _meanLocalMapUpdateDuration(0.0)
{
    // set random seed
    const uint seed = utils::Random::_seed;
    outputs::log(std::format("Constructed using seed {}", seed));
    std::srand(seed);
    cv::theRNG().state = seed;

    // Load parameters (once)
    if (not Parameters::is_valid())
    {
        Parameters::load_defaut();
        if (not Parameters::is_valid())
        {
            outputs::log_error("Invalid default parameters. Check your static parameters configuration");
            exit(-1);
        }
        outputs::log("Invalid parameters. Switching to default parameters");
    }

    // set threads
    const int availableCores = static_cast<int>(parameters::coreNumber);
    cv::setNumThreads(availableCores);
    Eigen::setNbThreads(availableCores);

    // primitive connected graph creator
    _depthOps = std::make_unique<features::primitives::Depth_Map_Transformation>(
            _width, _height, parameters::detection::depthMapPatchSize_px);
    if (_depthOps == nullptr)
    {
        outputs::log_error("Cannot create depth corrector, exiting");
        exit(-1);
    }

    // local map
    _localMap = std::make_unique<map_management::Local_Map>();

    // plane/cylinder finder
    _primitiveDetector = std::make_unique<features::primitives::Primitive_Detection>(_width, _height);

    // Point detector and matcher
    _pointDetector = std::make_unique<features::keypoints::Key_Point_Extraction>();

    // Line segment detector
    _lineDetector = std::make_unique<features::lines::Line_Detection>(0.3, 0.9);

    if (_primitiveDetector == nullptr)
    {
        outputs::log_error("Instanciation of Primitive_Detector failed");
        exit(-1);
    }
    if (_pointDetector == nullptr)
    {
        outputs::log_error("Instanciation of Key_Point_Extraction failed");
        exit(-1);
    }
    if (_lineDetector == nullptr)
    {
        outputs::log_error("Instanciation of Line_Detector failed");
        exit(-1);
    }

    _computeKeypointCount = 0;
    _currentPose = startPose;
}

void RGBD_SLAM::rectify_depth(cv::Mat_<float>& depthImage) noexcept
{
    cv::Mat_<float> rectifiedDepth;
    if (_depthOps->rectify_depth(depthImage, rectifiedDepth))
    {
        assert(depthImage.size == rectifiedDepth.size);
        depthImage = rectifiedDepth;
    }
    else
    {
        outputs::log_error("Could not rectify the depth image to rgb space");
    }
}

utils::Pose RGBD_SLAM::track(const cv::Mat& inputRgbImage, const cv::Mat_<float>& inputDepthImage) noexcept
{
    assert(static_cast<size_t>(inputDepthImage.rows) == _height);
    assert(static_cast<size_t>(inputDepthImage.cols) == _width);
    assert(static_cast<size_t>(inputRgbImage.rows) == _height);
    assert(static_cast<size_t>(inputRgbImage.cols) == _width);

    // project depth image in an organized cloud
    const double depthImageTreatmentStartTime = static_cast<double>(cv::getTickCount());
    // organized 3D depth image
    matrixf cloudArrayOrganized;
    const bool didOrganizedCloudArraySucceded =
            _depthOps->get_organized_cloud_array(inputDepthImage, cloudArrayOrganized);
    assert(didOrganizedCloudArraySucceded);
    _meanDepthMapTreatmentDuration +=
            (static_cast<double>(cv::getTickCount()) - depthImageTreatmentStartTime) / cv::getTickFrequency();

    // Compute a gray image for feature extractions
    cv::Mat grayImage;
    cv::cvtColor(inputRgbImage, grayImage, cv::COLOR_BGR2GRAY);

    // this frame points and  assoc
    const double computePoseStartTime = static_cast<double>(cv::getTickCount());
    const utils::Pose& refinedPose = this->compute_new_pose(grayImage, inputDepthImage, cloudArrayOrganized);
    _meanPoseOptimizationDuration +=
            (static_cast<double>(cv::getTickCount()) - computePoseStartTime) / cv::getTickFrequency();

    _totalFrameTreated += 1;
    return refinedPose;
}

cv::Mat RGBD_SLAM::get_debug_image(const utils::Pose& camPose,
                                   const cv::Mat& originalRGB,
                                   const double elapsedTime,
                                   const bool shouldDisplayStagedPoints,
                                   const bool shouldDisplayLineDetection,
                                   const bool shouldDisplayPrimitiveMasks) const noexcept
{
    // TODO: use shouldDisplayLineDetection
    cv::Mat debugImage = originalRGB.clone();

    const uint bandSize =
            static_cast<uint>(std::floor(_height / 25.0)); // 1/25 of the total image should be for the top black band

    // Show frame rate and labels
    cv::rectangle(debugImage,
                  cv::Point(0, 0),
                  cv::Point(static_cast<int>(_width), static_cast<int>(bandSize)),
                  cv::Scalar(0, 0, 0),
                  -1);
    if (elapsedTime > 0)
    {
        std::stringstream fps;
        fps << std::format("{: >3}", round(1 / elapsedTime + 0.5)) << " fps";
        cv::putText(
                debugImage, fps.str(), cv::Point(15, 15), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255, 1));
    }

    _localMap->get_debug_image(camPose, shouldDisplayStagedPoints, shouldDisplayPrimitiveMasks, debugImage);

    // display a red overlay if tracking is lost
    if (_isTrackingLost)
    {
        const cv::Size& debugImageSize = debugImage.size();
        cv::addWeighted(debugImage, 0.8, cv::Mat(debugImageSize, CV_8UC3, cv::Scalar(0, 0, 255)), 0.2, 1, debugImage);
    }

    return debugImage;
}

utils::Pose RGBD_SLAM::compute_new_pose(const cv::Mat& grayImage,
                                        const cv::Mat_<float>& depthImage,
                                        const matrixf& cloudArrayOrganized) noexcept
{
    if (not utils::is_covariance_valid(_currentPose.get_pose_variance()))
    {
        outputs::log_error("The current stored pose has an invalid covariance, system is broken");
        exit(-1);
    }
    // every now and then, restart the search of points even if we have enough features
    _computeKeypointCount = (_computeKeypointCount % parameters::detection::keypointRefreshFrequency) + 1;

// get a pose with the decaying motion model (do not add uncertainty if it's the first call)
#if 0 // TODO : put back when the motion model as been debugged
    const utils::Pose& predictedPose = _motionModel.predict_next_pose(_currentPose, not _isFirstTrackingCall);
#else
    const utils::Pose& predictedPose = _currentPose;
#endif

    // detect the features from the inputs
    const auto& detectedFeatures = detect_features(predictedPose, grayImage, depthImage, cloudArrayOrganized);

    // Find matches by the pose predicted by motion model
    const double findMatchesStartTime = static_cast<double>(cv::getTickCount());
    const matches_containers::matchContainer& matchedFeatures =
            _localMap->find_feature_matches(predictedPose, detectedFeatures);
    _meanFindMatchTime += (static_cast<double>(cv::getTickCount()) - findMatchesStartTime) / cv::getTickFrequency();

    // The new pose, after optimization
    utils::Pose newPose = predictedPose;

    // Optimize refined pose
    utils::Pose optimizedPose;
    matches_containers::match_sets matchSets;

    // optimize the pose, but not if it is the first call (no pose to compute)
    const double optimizePoseStartTime = static_cast<double>(cv::getTickCount());
    const bool isPoseValid =
            (not _isFirstTrackingCall) and pose_optimization::Pose_Optimization::compute_optimized_pose(
                                                   predictedPose, matchedFeatures, optimizedPose, matchSets);
    _meanPoseOptimizationFromFeatures +=
            (static_cast<double>(cv::getTickCount()) - optimizePoseStartTime) / cv::getTickFrequency();

    const double updateLocalMapStartTime = static_cast<double>(cv::getTickCount());
    if (isPoseValid)
    {
        // Update current pose if tracking is ongoing
        newPose = optimizedPose;
        _currentPose = optimizedPose;

        // Update local map if a valid transformation was found
        try
        {
            _localMap->update(
                    optimizedPose, detectedFeatures, matchSets._pointSets._outliers, matchSets._planeSets._outliers);
            _isTrackingLost = false;
            _failedTrackingCount = 0;
        }
        catch (const std::exception& ex)
        {
            outputs::log_error("Caught exeption while updating map: " + std::string(ex.what()));

            // no valid transformation
            _localMap->update_no_pose();

            _isTrackingLost = (++_failedTrackingCount) > 3;
            _motionModel.reset();
        }
    }
    // else the refined pose will follow the motion model
    else
    {
        // no valid transformation
        _localMap->update_no_pose();

        // add unmatched features if not tracking could be done last call
        const matrix33& poseCovariance = predictedPose.get_position_variance();
        if (_isTrackingLost and utils::is_covariance_valid(poseCovariance))
        {
            const CameraToWorldMatrix& cameraToWorld = utils::compute_camera_to_world_transform(
                    predictedPose.get_orientation_quaternion(), predictedPose.get_position());

            _localMap->add_features_to_map(poseCovariance, cameraToWorld, detectedFeatures, true);
        }

        if (not _isFirstTrackingCall)
        {
            // tracking is lost after some consecutive fails
            // TODO add to parameters
            _isTrackingLost = (++_failedTrackingCount) > 3;

            _motionModel.reset();

            outputs::log_error("Could not find an optimized pose");
        }
    }
    _meanLocalMapUpdateDuration +=
            (static_cast<double>(cv::getTickCount()) - updateLocalMapStartTime) / cv::getTickFrequency();

    // set firstCall to false after the first iteration
    _isFirstTrackingCall = false;

    return newPose;
}

map_management::DetectedFeatureContainer RGBD_SLAM::detect_features(const utils::Pose& predictedPose,
                                                                    const cv::Mat& grayImage,
                                                                    const cv::Mat_<float>& depthImage,
                                                                    const matrixf& cloudArrayOrganized) noexcept
{
#define USE_KEYPOINTS_DETECTION
#ifdef USE_KEYPOINTS_DETECTION
    // keypoint detection
    auto kpHandler = std::async(std::launch::async, [this, &predictedPose, &grayImage, &depthImage]() {
        const bool shouldRecomputeKeypoints = _isTrackingLost or _computeKeypointCount == 1;
        // Get map points that were tracked last call, and retroproject them to screen space using
        // last pose (used for optical flow)
        const features::keypoints::KeypointsWithIdStruct& trackedKeypointContainer =
                _localMap->get_tracked_keypoints_features(predictedPose);
        // Detect keypoints, and match the one detected by optical flow
        return _pointDetector->compute_keypoints(
                grayImage, depthImage, trackedKeypointContainer, shouldRecomputeKeypoints);
    });
#else
    auto kpHandler = std::async(std::launch::async, [&depthImage]() {
        static features::keypoints::Keypoint_Handler keypointHandler(depthImage.cols, depthImage.rows, 1.0);
        return keypointHandler;
    });
#endif

#define USE_PLANE_DETECTION
#ifdef USE_PLANE_DETECTION
    // plane detection
    auto planeHandler = std::async(std::launch::async, [this, &cloudArrayOrganized, &depthImage]() {
        // Run primitive detection
        const double primitiveDetectionStartTime = static_cast<double>(cv::getTickCount());
        features::primitives::plane_container detectedPlanes;
        features::primitives::cylinder_container detectedCylinders; // TODO: handle detected cylinders in local map
        _primitiveDetector->find_primitives(cloudArrayOrganized, depthImage, detectedPlanes, detectedCylinders);
        _meanPrimitiveTreatmentDuration +=
                (static_cast<double>(cv::getTickCount()) - primitiveDetectionStartTime) / cv::getTickFrequency();

        return detectedPlanes;
    });
#else
    auto planeHandler = std::async(std::launch::async, []() {
        return features::primitives::plane_container();
    });
#endif

#ifdef USE_LINE_DETECTION
    // line detection
    auto lineHandler = std::async([this, &grayImage, &depthImage]() {
        const double lineDetectionStartTime = static_cast<double>(cv::getTickCount());
        const features::lines::line_container& detectedLines = _lineDetector->detect_lines(grayImage, depthImage);
        _meanLineTreatmentDuration +=
                (static_cast<double>(cv::getTickCount()) - lineDetectionStartTime) / cv::getTickFrequency();

        return detectedLines;
    });
#else
    auto lineHandler = std::async([]() {
        return features::lines::line_container();
    });
#endif

    return map_management::DetectedFeatureContainer(kpHandler.get(), lineHandler.get(), planeHandler.get());
}

double get_percent_of_elapsed_time(const double treatmentTime, const double totalTimeElapsed) noexcept
{
    if (totalTimeElapsed <= 0)
        return 0;
    return std::round(treatmentTime / totalTimeElapsed * 10000) / 100;
}

void RGBD_SLAM::show_statistics(const double meanFrameTreatmentDuration) const noexcept
{
    if (_totalFrameTreated > 0)
    {
        const double pointCloudTreatmentDuration = _meanDepthMapTreatmentDuration / _totalFrameTreated;
        std::cout << "Mean image to point cloud treatment duration is " << pointCloudTreatmentDuration << " seconds ("
                  << get_percent_of_elapsed_time(pointCloudTreatmentDuration, meanFrameTreatmentDuration) << "%)"
                  << std::endl;
        const double poseTreatmentDuration = _meanPoseOptimizationDuration / _totalFrameTreated;
        std::cout << "Mean pose estimation duration is " << poseTreatmentDuration << " seconds ("
                  << get_percent_of_elapsed_time(poseTreatmentDuration, meanFrameTreatmentDuration) << "%)"
                  << std::endl;

        std::cout << std::endl;
        std::cout << "Pose optimization profiling details:" << std::endl;
        // display primitive detection statistic
        const double primitiveTreatmentDuration = _meanPrimitiveTreatmentDuration / _totalFrameTreated;
        std::cout << "\tMean primitive treatment duration is " << primitiveTreatmentDuration << " seconds ("
                  << get_percent_of_elapsed_time(primitiveTreatmentDuration, meanFrameTreatmentDuration) << "%)"
                  << std::endl;
        // display line detection statistics
        const double lineDetectionDuration = _meanLineTreatmentDuration / _totalFrameTreated;
        std::cout << "\tMean line detection duration is " << lineDetectionDuration << " seconds ("
                  << get_percent_of_elapsed_time(lineDetectionDuration, meanFrameTreatmentDuration) << "%)"
                  << std::endl;
        // display point detection statistics
        _pointDetector->show_statistics(meanFrameTreatmentDuration, _totalFrameTreated);
        // display find match statistics
        const double findMatchDuration = _meanFindMatchTime / _totalFrameTreated;
        std::cout << "\tMean find match duration is " << findMatchDuration << " seconds ("
                  << get_percent_of_elapsed_time(findMatchDuration, meanFrameTreatmentDuration) << "%)" << std::endl;
        // display pose optimization from features statistics
        const double poseOptimizationDuration = _meanPoseOptimizationFromFeatures / _totalFrameTreated;
        std::cout << "\tMean pose optimization duration is " << poseOptimizationDuration << " seconds ("
                  << get_percent_of_elapsed_time(poseOptimizationDuration, meanFrameTreatmentDuration) << "%)"
                  << std::endl;
        // display local map update statistics
        const double localMapUpdateDuration = _meanLocalMapUpdateDuration / _totalFrameTreated;
        std::cout << "\tMean local map update duration is " << localMapUpdateDuration << " seconds ("
                  << get_percent_of_elapsed_time(localMapUpdateDuration, meanFrameTreatmentDuration) << "%)"
                  << std::endl;
    }
}

} // namespace rgbd_slam
