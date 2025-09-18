#pragma once
#include "landmark_manager.hpp"
#include <d2common/d2state.hpp>
#include <d2common/d2vinsframe.h>

using namespace Eigen;
using namespace D2Common;

namespace D2VINS {
class Marginalizer;
class PriorFactor;
using PriorFactorPtr = std::shared_ptr<PriorFactor>;

class D2EstimatorState : public D2State {
protected:
    std::map<int, std::vector<VINSFramePtr>> sld_wins;
    std::map<int, std::vector<FrameIdType>> latest_remote_sld_wins;
    std::map<FrameIdType, int> frame_indices;
    D2LandmarkManager lmanager;
    std::map<FrameIdType, StatePtr> _frame_spd_Bias_state;
    std::map<CamIdType, StatePtr> _camera_extrinsic_state;
    std::vector<CamIdType> local_camera_ids;
    std::map<CamIdType, int> camera_drone;
    std::map<CamIdType, Swarm::Pose> extrinsic; //extrinsic of cameras by ID
    std::map<FrameIdType, VectorXd> linear_point;
    std::map<FrameIdType, Swarm::Odometry> ego_motions;
    FrameIdType last_ego_frame_id;

    Marginalizer * marginalizer = nullptr;
    PriorFactorPtr prior_factor = nullptr;

    std::vector<LandmarkPerId> popFrame(int index);
    std::vector<LandmarkPerId> removeFrameById(FrameIdType frame_id, bool remove_base=false); 
        //If remove base, will remove the relevant landmarks' base frame.
        //This is for marginal the keyframes that not is baseframe of all landmarks (in multi-drone)
    void outlierRejection(const std::set<LandmarkIdType> & used_landmarks);
    void updateSldWinsIMU(const std::map<int, IMUBuffer> & remote_imu_bufs);
    void createPriorFactor4FirstFrame(const VINSFramePtr& frame);
    bool solveGyroscopeBias(const std::vector<VINSFramePtr>& sld_win, const std::map<FrameIdType, Swarm::Pose>& sfm_poses, Swarm::Pose extrinsic);
    bool LinearAlignment(const std::vector<VINSFramePtr>& sld_win, 
        const std::map<FrameIdType, Swarm::Pose>& sfm_poses, Swarm::Pose extrinsic);
    void RefineGravity(const std::vector<VINSFramePtr>& sld_win, 
        const std::map<FrameIdType, Swarm::Pose>& sfm_poses, Swarm::Pose extrinsic, Vector3d &g, VectorXd &x);

    Vector3d Ba = Vector3d::Zero();
    Vector3d Bg = Vector3d::Zero();
    StatePtr td = nullptr;
public:
    D2EstimatorState(int _self_id);

    void init(std::vector<Swarm::Pose> _extrinsic, double _td);

    //Get states
    int getPoseIndex(FrameIdType frame_id) const;
    StatePtr getExtrinsicState(int i) const;
    StatePtr getSpdBiasState(FrameIdType frame_id) const;
    StatePtr getLandmarkState(LandmarkIdType landmark_id) const;
    StatePtr getTdState(int drone_id);
    double getTd(int drone_id) const;
    double getTd() const;
    PriorFactorPtr getPrior() const;
    FrameIdType getLandmarkBaseFrame(LandmarkIdType landmark_id) const;
    Swarm::Pose getExtrinsic(CamIdType cam_id) const;
    std::set<CamIdType> getAvailableCameraIds() const;
    std::vector<LandmarkPerId> availableLandmarkMeasurements(int max_pts, int max_measurement) const;
    std::vector<LandmarkPerId> getInitializedLandmarks() const;
    LandmarkPerId & getLandmarkbyId(LandmarkIdType id);
    bool hasLandmark(LandmarkIdType id) const;

    //Camera
    CamIdType addCamera(const Swarm::Pose & pose, int camera_index, int drone_id, CamIdType camera_id=-1);
    int getCameraBelonging(CamIdType cam_id) const;
    bool hasCamera(CamIdType frame_id) const;
    std::vector<Swarm::Pose> localCameraExtrinsics() const;
   
    //Frame operations
    std::vector<LandmarkPerId> clearUselessFrames(bool marginalization=true);
    void addFrame(const VisualImageDescArray & images, const VINSFramePtr & _frame);
    void updateSldwin(int drone_id, const std::vector<FrameIdType> & sld_win);
    virtual void moveAllPoses(int new_ref_frame_id, const Swarm::Pose & delta_pose) override;
    const std::vector<VINSFramePtr> & getSldWin(int drone_id) const;
    void addVINSFrame(const VINSFramePtr & _frame);

    //Frame access    
    const std::shared_ptr<VINSFrame> getVINSFramebyId(FrameIdType frame_id) const;
    std::shared_ptr<VINSFrame> getVINSFramebyId(FrameIdType frame_id);
    std::shared_ptr<VINSFrame> getFrame(int index);
    const std::shared_ptr<VINSFrame> getFrame(int index) const;
    const std::shared_ptr<VINSFrame> getFrame(int drone_id, int index) const;
    std::shared_ptr<VINSFrame> getFrame(int drone_id, int index);
    std::shared_ptr<VINSFrame> firstFrame();
    std::shared_ptr<VINSFrame> firstFrame(int drone_id);
    const std::shared_ptr<VINSFrame> lastFrame() const;
    std::shared_ptr<VINSFrame> lastFrame();
    std::shared_ptr<VINSFrame> lastFrame(int drone_id);
    const std::shared_ptr<VINSFrame> lastFrame(int drone_id) const;
    size_t size() const;
    Swarm::Pose getEstimatedPose(int drone_id, int index) const;
    Swarm::Pose getEstimatedPose(FrameIdType frame_id) const;
    Swarm::Odometry getEstimatedOdom(FrameIdType frame_id) const;
    size_t size(int drone_id) const;

    //Solving process
    void syncFromState(const std::set<LandmarkIdType> & used_landmarks);
    void preSolve(const std::map<int, IMUBuffer> & remote_imu_bufs);
    void repropagateIMU();
    void setPose(FrameIdType frame_id, const Swarm::Pose & pose);
    void setVelocity(FrameIdType frame_id, const Vector3d & velocity);
    void setBias(FrameIdType frame_id, const Vector3d & ba, const Vector3d & bg);

    Eigen::Vector3d getBa() const {
        return Ba;
    }

    Eigen::Vector3d getBg() const {
        return Bg;
    }

    int numKeyframes() const;

    //Debug
    void printSldWin(const std::map<FrameIdType, int> & keyframe_measurments) const;

    void setMarginalizer(Marginalizer * _marginalizer) {
        marginalizer = _marginalizer;
    }
    const std::map<LandmarkIdType, LandmarkPerId> & getLandmarkDB() const {
        return lmanager.getLandmarkDB();
    }

    void updateEgoMotion();
    void printLandmarkReport(FrameIdType frame_id) const;
    bool monoInitialization();

};
}