#include "PoseOptimization.hpp"

#include "utils.hpp"
#include "LevenbergMarquardFunctors.hpp"
#include "parameters.hpp"

#include <Eigen/StdVector>

namespace rgbd_slam {
    namespace pose_optimization {

        /**
         *
         * \param[in] optimizedPose the pose obtained after one optimization
         * \param[in] matchedPoints the original matched point container
         * \param[out] retroprojectionErrorContainer A container with all the errors of the matched points
         *
         * \return the mean of the error vector
         */
        double get_retroprojection_error_vector(const utils::Pose& optimizedPose, const match_point_container& matchedPoints, std::vector<double>& retroprojectionErrorContainer)
        {
            retroprojectionErrorContainer.reserve(matchedPoints.size());

            double mean = 0;
            const matrix34& newTransformationMatrix = utils::compute_world_to_camera_transform(optimizedPose.get_orientation_quaternion(), optimizedPose.get_position());
            for(match_point_container::const_iterator pointIterator = matchedPoints.cbegin(); pointIterator != matchedPoints.cend(); ++pointIterator) {
                const vector2& screenPoint = vector2(pointIterator->first.x(), pointIterator->first.y());
                const vector3& worldPoint = pointIterator->second;

                const vector2& projectedPoint = utils::world_to_screen_coordinates(worldPoint, newTransformationMatrix);
                const double projectionError = (projectedPoint - screenPoint).norm();
                mean += projectionError;
                retroprojectionErrorContainer.push_back(projectionError);
            }
            mean /= retroprojectionErrorContainer.size();
            return mean;
        }

        /**
         * \brief Remove the outliers of the matched point container by excluding the 5% of errors
         *
         * \param[in] matchedPoints the original matched point container
         * \param[in] retroprojectionErrorContainer error of the retroprojection
         * \param[in] retroprojectionErrorMean mean of the error vector
         *
         * \return a matched point container without outliers
         */
        const match_point_container remove_match_outliers(const match_point_container& matchedPoints, const std::vector<double>& retroprojectionErrorContainer, const double retroprojectionErrorMean)
        {
            double variance = 0;
            for(const double& error : retroprojectionErrorContainer) 
            {
                variance += std::pow(error - retroprojectionErrorMean, 2.0);
            }
            variance /= retroprojectionErrorContainer.size();
            const double standardDeviation = std::max(sqrt(variance), 0.5);

            // standard threshold: 
            const double stdError = standardDeviation * 1.5;  // 95% not outliers
            const double highThresh = retroprojectionErrorMean + stdError;
            const double lowThresh =  retroprojectionErrorMean - stdError;

            match_point_container newMatchedPoints;
            size_t cnt = 0;
            for(match_point_container::const_iterator pointIterator = matchedPoints.cbegin(); pointIterator != matchedPoints.cend(); ++pointIterator, ++cnt) {
                if (retroprojectionErrorContainer[cnt] <= highThresh and retroprojectionErrorContainer[cnt] >= lowThresh)
                {
                    const vector3& screenPoint = pointIterator->first;
                    const vector3& worldPoint = pointIterator->second;

                    newMatchedPoints.emplace(newMatchedPoints.end(), screenPoint, worldPoint);
                }
            }

            return newMatchedPoints;
        }

        const utils::Pose Pose_Optimization::compute_optimized_pose(const utils::Pose& currentPose, const match_point_container& matchedPoints) 
        {
            // get absolute displacement error
            const double maximumOptimizationRetroprojection = Parameters::get_maximum_optimization_retroprojection_error();
            const size_t maximumOptimizationReiteration = Parameters::get_maximum_optimization_reiteration();
            const size_t minimumPointsForOptimization = Parameters::get_minimum_point_count_for_optimization();

            utils::Pose finalPose = currentPose;
            match_point_container finalMatchedPoints = matchedPoints;
            double errorOfSet = 0.0;
            size_t iterationCount = 0;
            while (iterationCount < maximumOptimizationReiteration)
            {
                ++iterationCount;
                const utils::Pose& optimizedPose = get_optimized_global_pose(currentPose, finalMatchedPoints);

                std::vector<double> retroprojectionErrorContainer;
                const double meanOfErrorContainer = get_retroprojection_error_vector(currentPose, finalMatchedPoints, retroprojectionErrorContainer);
                errorOfSet = meanOfErrorContainer;

                const match_point_container& newMatchedPoints = remove_match_outliers(finalMatchedPoints, retroprojectionErrorContainer, meanOfErrorContainer);

                finalPose = optimizedPose;
                if (errorOfSet < maximumOptimizationRetroprojection)
                {
                    finalMatchedPoints = newMatchedPoints;
                    break;
                }
                else if (newMatchedPoints.size() == finalMatchedPoints.size())
                {
                    //utils::log_error("No outliers found in original set, but optimization error is too great");
                    break;
                }
                else if (newMatchedPoints.size() <= minimumPointsForOptimization)
                {
                    utils::log_error("Not enough points for pose optimization after outlier removal");
                    break;
                }
                finalMatchedPoints = newMatchedPoints;
            } 

            //std::cout << "Final score is " << errorOfSet << " after " << iterationCount << " iterations (optimization with " << finalMatchedPoints.size() << "/" << matchedPoints.size() << " points)" << std::endl;
            if (errorOfSet > maximumOptimizationRetroprojection)
            {
                utils::log_error("Optimization error is too high");
                return finalPose;
            }
            return finalPose;
        }


        const utils::Pose Pose_Optimization::get_optimized_global_pose(const utils::Pose& currentPose, const match_point_container& matchedPoints) 
        {
            const vector3& position = currentPose.get_position();    // Work in millimeters
            const quaternion& rotation = currentPose.get_orientation_quaternion();

            // Vector to optimize: (0, 1, 2) is position,
            // Vector (3, 4, 5) is a rotation parametrization, representing a delta in rotation in the tangential hyperplane -From Using Quaternions for Parametrizing 3-D Rotation in Unconstrained Nonlinear Optimization)
            Eigen::VectorXd input(6);
            // 3D pose
            input[0] = position.x();
            input[1] = position.y();
            input[2] = position.z();
            // X Y Z of a quaternion representation (0, 0, 0) corresponds to the quaternion itself
            const vector3& rotationCoefficients = get_scaled_axis_coefficients_from_quaternion(rotation);
            input[3] = rotationCoefficients.x();
            input[4] = rotationCoefficients.y();
            input[5] = rotationCoefficients.z();

            // Optimize function 
            Global_Pose_Functor pose_optimisation_functor(
                    Global_Pose_Estimator(
                        input.size(), 
                        matchedPoints, 
                        currentPose.get_position(),
                        currentPose.get_orientation_quaternion()
                        )
                    );
            // Optimization algorithm
            Eigen::LevenbergMarquardt<Global_Pose_Functor, double> poseOptimizator( pose_optimisation_functor );

            // maxfev   : maximum number of function evaluation
            // xtol     : tolerance for the norm of the solution vector
            // ftol     : tolerance for the norm of the vector function
            // gtol     : tolerance for the norm of the gradient of the error function
            // factor   : step bound for the diagonal shift
            // epsfcn   : error precision
            poseOptimizator.parameters.maxfev = Parameters::get_optimization_maximum_iterations();
            poseOptimizator.parameters.epsfcn = Parameters::get_optimization_error_precision();
            poseOptimizator.parameters.xtol = Parameters::get_optimization_xtol();
            poseOptimizator.parameters.ftol = Parameters::get_optimization_ftol();
            poseOptimizator.parameters.gtol = Parameters::get_optimization_gtol();
            poseOptimizator.parameters.factor = Parameters::get_optimization_factor();


            const Eigen::LevenbergMarquardtSpace::Status endStatus = poseOptimizator.minimize(input);

            const quaternion& endRotation = get_quaternion_from_scale_axis_coefficients(
                    vector3(
                        input[3],
                        input[4],
                        input[5]
                        )
                    ); 
            const vector3 endPosition(
                    input[0],
                    input[1],
                    input[2]
                    );

            if (endStatus == Eigen::LevenbergMarquardtSpace::Status::TooManyFunctionEvaluation)
            {
                // Error: reached end of minimization without reaching a minimum
                const std::string message = get_human_readable_end_message(endStatus);
                utils::log("Failed to converge with " + std::to_string(matchedPoints.size()) + " points | Status " + message);
            }

            // Update refine pose with optimized pose
            return utils::Pose(endPosition, endRotation);
        }

    }   /* pose_optimization*/
}   /* rgbd_slam */
