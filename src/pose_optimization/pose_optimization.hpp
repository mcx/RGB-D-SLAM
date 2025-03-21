#ifndef RGBDSLAM_POSEOPTIMIZATION_POSEOPTIMIZATION_HPP
#define RGBDSLAM_POSEOPTIMIZATION_POSEOPTIMIZATION_HPP

#include "matches_containers.hpp"
#include "utils/pose.hpp"

namespace rgbd_slam::pose_optimization {

/**
 * \brief Find the transformation between a matches feature sets, using a custom Levenberg Marquardt method
 */
class Pose_Optimization
{
  public:
    /**
     * \brief Compute a new observer global pose, to replace the current estimated pose
     *
     * \param[in] currentPose Last observer optimized pose
     * \param[in] matchedFeatures Object containing the match between observed screen features and reliable map features
     * \param[out] optimizedPose The estimated world translation & rotation of the camera pose, if the function returned
     * true
     * \param[out] featureSets The inliers/outliers matched features for the finalPose. Valid if the function returned
     * true
     *
     * \return True if a valid pose was computed
     */
    [[nodiscard]] static bool compute_optimized_pose(const utils::PoseBase& currentPose,
                                                     const matches_containers::match_container& matchedFeatures,
                                                     utils::Pose& optimizedPose,
                                                     matches_containers::match_sets& featureSets) noexcept;

    /**
     * \brief Compute the variance of a given pose, using multiple iterations of the optimization process
     *
     * \param[in] optimizedPose The pose to compute the variance of
     * \param[in] matchedFeatures The features used to compute this pose
     * \param[out] poseCovariance The computed covariance, only valid if this function returns true
     * \param[in] iterations The number of optimization iterations. The higher the better the estimation, but it gets
     * less and less efficient
     *
     * \return True if the process succeded, or False
     */
    [[nodiscard]] static bool compute_pose_variance(const utils::PoseBase& optimizedPose,
                                                    const matches_containers::match_container& matchedFeatures,
                                                    matrix66& poseCovariance,
                                                    const uint iterations = 100) noexcept;

    static void show_statistics(const double meanFrameTreatmentDuration,
                                const uint frameCount,
                                const bool shouldDisplayDetails = false) noexcept;

  private:
    /**
     * \brief Optimize a global pose (orientation/translation) of the observer, given a match set
     *
     * \param[in] currentPose Last observer optimized pose
     * \param[in] matchedFeatures Object containing the match between observed screen features and reliable map features
     * \param[out] optimizedPose The estimated world translation & rotation of the camera pose, if the function returned
     * true
     *
     * \return True if a valid pose was computed
     */
    [[nodiscard]] static bool compute_optimized_global_pose(const utils::PoseBase& currentPose,
                                                            const matches_containers::match_container& matchedFeatures,
                                                            utils::PoseBase& optimizedPose) noexcept;

    /**
     * \brief Compute an optimized pose, using a RANSAC methodology
     *
     * \param[in] currentPose The current pose of the observer
     * \param[in] matchedFeatures Object container the match between observed screen features and local map features
     * \param[out] finalPose The optimized pose, valid if the function returned true
     * \param[out] featureSets The matched features detected as inlier and outliers. Valid if the function returned true
     *
     * \return True if a valid pose and inliers were found
     */
    [[nodiscard]] static bool compute_pose_with_ransac(const utils::PoseBase& currentPose,
                                                       const matches_containers::match_container& matchedFeatures,
                                                       utils::PoseBase& finalPose,
                                                       matches_containers::match_sets& featureSets) noexcept;

    /**
     * \brief
     *
     * \param[in] currentPose The pose to recompute
     * \param[in] matchedFeatures the feature set to make variations of
     * \param[out] optimizedPose The pose optimized with the variated feature set, if the functon returned true
     *
     * \return True if the new pose optimization is succesful, or False
     */
    [[nodiscard]] static bool compute_random_variation_of_pose(
            const utils::PoseBase& currentPose,
            const matches_containers::match_container& matchedFeatures,
            utils::PoseBase& optimizedPose) noexcept;

    // perf monitoring
    inline static double _meanPoseRANSACDuration = 0.0;
    inline static double _meanComputePoseVarianceDuration = 0.0;

    inline static double _meanGetRandomSubsetDuration = 0.0;
    inline static double _meanRANSACPoseOptimizationDuration = 0.0;
    inline static double _meanRANSACGetInliersDuration = 0.0;
};

} // namespace rgbd_slam::pose_optimization

#endif
