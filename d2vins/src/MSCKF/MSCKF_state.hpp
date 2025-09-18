#include "swarm_msgs/Pose.h"

#define IMU_STATE_DIM 15
#define IMU_NOISE_DIM 12

class MSCKFStateVector {
public:
    // Follow param should be
    //q, bias_gyro, v, bias_acc, p, [q_cam, p_cam], [q_t-1,p_t-1] ... [q_t-n, p_t-n]
    Quaterniond q_imu; //quaternion in global frame
    Vector3d bias_gyro; //bias in body frame
    Vector3d v_imu; //Velocity of Imu in global frame
    Vector3d bias_acc; //bias of acceleration
    Vector3d p_imu; //position
    std::vector<Swarm::Pose> camera_extrisincs; //[q, p]^T
    std::vector<Swarm::Pose> sld_win_poses;  //[q_{t-1},p_{t-1}], [q_{t-2},p_{t-2}] .., [q_{t-n},p_{t-n}]


    void addKeyframe(double t);
    
    MSCKFStateVector();
    Eigen::VectorXd getFullVector();
    Matrix3d get_imu_R() const;
};

class MSCKFErrorStateVector {
public:
    // Follow param is ordered exactly as them in vector [ang, bias_gyro, v_imu, bias_acc, pos]
    Vector3d ang, bias_gyro;
    Vector3d v_imu;
    Vector3d bias_acc, pos; //IMU vector should be these 15

    std::vector<std::pair<Vector3d, Vector3d>>  camera_extrisincs; //In q, P order
    //IMU poses to here...

    std::vector<std::pair<Vector3d, Vector3d>>  sld_win_poses; //In q, P order
    
    MatrixXd P;

    MSCKFErrorStateVector() {}

    MSCKFErrorStateVector(const MSCKFStateVector & state0);
    
    void stateAugmentation(double t);
    
    VectorXd getFullVector() const;
    Eigen::Matrix<double, IMU_STATE_DIM, 1> getImuVector() const;
    Eigen::Matrix<double, IMU_STATE_DIM, IMU_STATE_DIM> getImuP() const;
    Eigen::Matrix<double, IMU_STATE_DIM, Eigen::Dynamic> getImuOtherP() const;

    void setImuVector(Eigen::Matrix<double, IMU_STATE_DIM, 1> v);
    void setImuP(Eigen::Matrix<double, IMU_STATE_DIM, IMU_STATE_DIM> _P);
    void setImuOtherP(Eigen::Matrix<double, IMU_STATE_DIM, Eigen::Dynamic> _P);
    
    void reset(MSCKFStateVector & state); //Reset with MSCKF state

    unsigned int stateDimFull() {
        return IMU_STATE_DIM + camera_extrisincs.size() * 6 + sld_win_poses.size() * 6;
    }

};