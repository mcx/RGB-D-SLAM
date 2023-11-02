#ifndef RGBDSLAM_MAPMANAGEMENT_MAPPOINT2D_HPP
#define RGBDSLAM_MAPMANAGEMENT_MAPPOINT2D_HPP

#include "feature_map.hpp"
#include "features/keypoints/keypoint_handler.hpp"
#include "tracking/point_with_tracking.hpp"
#include "matches_containers.hpp"

namespace rgbd_slam::map_management {

struct Point2D
{
    // world coordinates
    utils::ScreenCoordinate2D _coordinates;
    // 3D descriptor (ORB)
    cv::Mat _descriptor;
    // position covariance
    ScreenCoordinate2DCovariance _covariance;

    Point2D(const utils::ScreenCoordinate2D& coordinates,
            const ScreenCoordinate2DCovariance& covariance,
            const cv::Mat& descriptor);
};

struct UpgradedPoint2D
{
    utils::WorldCoordinate _coordinates;
    WorldCoordinateCovariance _covariance;
    cv::Mat _descriptor;
    int _matchIndex;
};

using DetectedKeypointsObject = features::keypoints::Keypoint_Handler;
using DetectedPoint2DType = features::keypoints::DetectedKeyPoint;
using PointMatch2DType = matches_containers::PointMatch2D;
using TrackedPointsObject = features::keypoints::KeypointsWithIdStruct;
using UpgradedPoint2DType = UpgradedPoint2D; // 2D points can be upgraded to 3D

class MapPoint2D :
    public Point2D,
    public IMapFeature<DetectedKeypointsObject,
                       DetectedPoint2DType,
                       PointMatch2DType,
                       TrackedPointsObject,
                       UpgradedPoint2DType>
{
  public:
    MapPoint2D(const utils::ScreenCoordinate2D& coordinates,
               const ScreenCoordinate2DCovariance& covariance,
               const cv::Mat& descriptor) :
        Point2D(coordinates, covariance, descriptor),
        IMapFeature<DetectedKeypointsObject,
                    DetectedPoint2DType,
                    PointMatch2DType,
                    TrackedPointsObject,
                    UpgradedPoint2DType>()
    {
        assert(_id > 0);
    }

    MapPoint2D(const utils::ScreenCoordinate2D& coordinates,
               const ScreenCoordinate2DCovariance& covariance,
               const cv::Mat& descriptor,
               const size_t id) :
        Point2D(coordinates, covariance, descriptor),
        IMapFeature<DetectedKeypointsObject,
                    DetectedPoint2DType,
                    PointMatch2DType,
                    TrackedPointsObject,
                    UpgradedPoint2DType>(id)
    {
        assert(_id > 0);
    }

    virtual ~MapPoint2D() = default;

    [[nodiscard]] int find_match(const DetectedKeypointsObject& detectedFeatures,
                                 const WorldToCameraMatrix& worldToCamera,
                                 const vectorb& isDetectedFeatureMatched,
                                 std::list<PointMatch2DType>& matches,
                                 const bool shouldAddToMatches = true,
                                 const bool useAdvancedSearch = false) const noexcept override;

    [[nodiscard]] bool add_to_tracked(const WorldToCameraMatrix& worldToCamera,
                                      TrackedPointsObject& trackedFeatures,
                                      const uint dropChance = 1000) const noexcept override;

    void draw(const WorldToCameraMatrix& worldToCamMatrix,
              cv::Mat& debugImage,
              const cv::Scalar& color) const noexcept override;

    [[nodiscard]] bool is_visible(const WorldToCameraMatrix& worldToCamMatrix) const noexcept override;

    void write_to_file(std::shared_ptr<outputs::IMap_Writer> mapWriter) const noexcept override;

    [[nodiscard]] bool compute_upgraded(const matrix33& poseCovariance,
                                        UpgradedPoint2DType& upgradeFeature) const noexcept override;

    WorldToCameraMatrix _firstWorldToCam;

    bool _isLastMatchCoordinatesSet = false;
    utils::ScreenCoordinate _lastMatchCoordinates; // can be 2D
    WorldToCameraMatrix _lastMatchWorldToCamera;

  protected:
    [[nodiscard]] bool update_with_match(const DetectedPoint2DType& matchedFeature,
                                         const matrix33& poseCovariance,
                                         const CameraToWorldMatrix& cameraToWorld) noexcept override;

    void update_no_match() noexcept override;
};

/**
 * \brief Candidate for a map point
 */
class StagedMapPoint2D : public MapPoint2D, public IStagedMapFeature<DetectedPoint2DType>
{
  public:
    StagedMapPoint2D(const matrix33& poseCovariance,
                     const CameraToWorldMatrix& cameraToWorld,
                     const DetectedPoint2DType& detectedFeature);

    [[nodiscard]] bool should_remove_from_staged() const noexcept override;

    [[nodiscard]] bool should_add_to_local_map() const noexcept override;

    [[nodiscard]] static bool can_add_to_map(const DetectedPoint2DType& detectedPoint) noexcept
    {
        return not detectedPoint._descriptor.empty() and not utils::is_depth_valid(detectedPoint._coordinates.z());
    }

  protected:
    double get_confidence() const noexcept;
};

/**
 * \brief A map point structure, containing all the necessary informations to identify a map point in local map
 */
class LocalMapPoint2D : public MapPoint2D, public ILocalMapFeature<StagedMapPoint2D>
{
  public:
    explicit LocalMapPoint2D(const StagedMapPoint2D& stagedPoint);

    [[nodiscard]] bool is_lost() const noexcept override;
};

using localPoint2DMap = Feature_Map<LocalMapPoint2D,
                                    StagedMapPoint2D,
                                    DetectedKeypointsObject,
                                    DetectedPoint2DType,
                                    PointMatch2DType,
                                    TrackedPointsObject,
                                    UpgradedPoint2DType>;

} // namespace rgbd_slam::map_management

#endif
