#include "parameters.hpp"

#include <math.h>
#include <cfloat>

namespace rgbd_slam {

    bool Parameters::parse_file(const std::string& fileName )
    {
        load_defaut();
        _isValid = true;
        // TODO
        return _isValid;
    }

    void Parameters::load_defaut() 
    {
        // Initial position & rotation, if necessary
        _startingPositionX = 0;
        _startingPositionY = 0;
        _startingPositionZ = 0;

        _startingRotationX = 0;
        _startingRotationY = 0;
        _startingRotationZ = 0;

        // Camera intrinsic parameters
        _camera1FocalX = 548.86723733696215;
        _camera1FocalY = 549.58402532237187;
        _camera1CenterX = 316.49655835885483;
        _camera1CenterY = 229.23873484682150;

        _camera2FocalX = 575.92685448804468;
        _camera2FocalY = 576.40791601093247;
        _camera2CenterX = 315.15026356388171;
        _camera2CenterY = 230.58580662101753;

        // Camera 2 position & rotation
        _camera2TranslationX = 1.1497548441022023e+01;
        _camera2TranslationY = 3.5139088879273231e+01;
        _camera2TranslationZ = 2.1887459420807019e+01;

        _camera2RotationX = 0; 
        _camera2RotationY = 0; 
        _camera2RotationZ = 0; 

        // Point detection/Matching
        _matchSearchRadius = 30;
        _matchSearchCellSize = 50;
        _maximumMatchDistance = 0.7;    // The closer to 0, the more discriminating
        _detectorMinHessian = 40;       // The higher the least detected points
        _keypointRefreshFrequency = 5; // Update the keypoint list every N calls
        _opticalFlowPyramidDepth = 5;   // depth of the optical pyramid
        _opticalFlowPyramidWindowSize = 25;
        _opticalFlowMaxError = 35;      // error in pixel after which a point is rejected
        _opticalFlowMaxDistance = 100;  // distance in pixel after which a point is rejected
        _keypointMaskDiameter = 30;     // do not detect points inside an area of this size (pixels) around existing keypoints

        // Pose Optimization
        _minimumPointForOptimization = 5;
        _maximumPointPerFrame = 100;
        _optimizationMaximumIterations = 1024;
        _optimizationErrorPrecision = 0;
        _optimizationToleranceOfSolutionVectorNorm = 1e-4;//sqrt(DBL_EPSILON); // Smallest delta of doubles
        _optimizationToleranceOfVectorFunction = 1e-3;
        _optimizationToleranceOfErrorFunctionGradient = 0;
        _optimizationDiagonalStepBoundShift = 100;
        _maximumRetroprojectionError = 10;
        _maximumRetroprojectionReiteration = 15;

        _pointWeightThreshold = 1.345;
        _pointWeightCoefficient = 1.4826;
        _pointLossAlpha = 2;    // -infinity, infinity
        _pointLossScale = 100; // Unit: Pixel
        _pointErrorMultiplier = 0.5;  // > 0

        // Local map
        _pointUnmatchedCountToLoose = 10;
        _pointAgeConfidence = 15;
        _pointStagedAgeConfidence = 10;
        _pointMinimumConfidenceForMap = 0.9;
        _mapMaximumRetroprojectionError = 150;

        // Primitive extraction
        _primitiveMaximumCosAngle = cos(M_PI/10.0);
        _primitiveMaximumMergeDistance = 100;
        _depthMapPatchSize = 20;

        _minimumPlaneSeedCount = 6;
        _minimumCellActivated = 5;
        _depthSigmaError = 1.425e-6;
        _depthSigmaMargin = 12;
        _depthDiscontinuityLimit = 10;
        _depthAlpha = 0.06;

        // Cylinder ransac fitting
        _cylinderRansacSqrtMaxDistance = 0.04;
        _cylinderRansacMinimumScore = 75;

        _isValid = true;
    }

};  /* rgbd_slam */
