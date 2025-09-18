#include <d2common/d2vinsframe.h>
#include <d2common/integration_base.h>

namespace D2Common {
double t0 = 0;
VINSFrame::VINSFrame(const VisualImageDescArray& frame, const IMUBuffer& buf,
                     const std::shared_ptr<VINSFrame>& prev_frame)
    : D2BaseFrame(frame.stamp, frame.frame_id, frame.drone_id,
                  frame.reference_frame_id, frame.is_keyframe,
                  frame.pose_drone),
      Ba(prev_frame->Ba),
      Bg(prev_frame->Bg),
      prev_frame_id(prev_frame->frame_id) {
  pre_integrations = std::make_shared<IntegrationBase>(buf, Ba, Bg);
  if (t0 == 0) {
    t0 = stamp;
  }
}

VINSFrame::VINSFrame(const VisualImageDescArray& frame,
                     const std::pair<IMUBuffer, int>& buf,
                     const std::shared_ptr<VINSFrame>& prev_frame)
    : D2BaseFrame(frame.stamp, frame.frame_id, frame.drone_id,
                  frame.reference_frame_id, frame.is_keyframe,
                  frame.pose_drone),
      Ba(prev_frame->Ba),
      Bg(prev_frame->Bg),
      prev_frame_id(prev_frame->frame_id),
      imu_buf_index(buf.second) {
  pre_integrations = std::make_shared<IntegrationBase>(buf.first, Ba, Bg);
  if (t0 == 0) {
    t0 = stamp;
  }
}

VINSFrame::VINSFrame(const VisualImageDescArray& frame, const Vector3d& _Ba,
                     const Vector3d& _Bg)
    : D2BaseFrame(frame.stamp, frame.frame_id, frame.drone_id,
                  frame.reference_frame_id, frame.is_keyframe,
                  frame.pose_drone),
      Ba(_Ba),
      Bg(_Bg) {
  if (t0 == 0) {
    t0 = stamp;
  }
}

VINSFrame::VINSFrame(const VisualImageDescArray& frame)
    : D2BaseFrame(frame.stamp, frame.frame_id, frame.drone_id,
                  frame.reference_frame_id, frame.is_keyframe,
                  frame.pose_drone),
      Ba(0, 0., 0.),
      Bg(0., 0., 0.) {
  if (t0 == 0) {
    t0 = stamp;
  }
}

std::string VINSFrame::toStr() {
  char buf[1024] = {0};
  char buf_imu[1024] = {0};
  if (pre_integrations != nullptr) {
    sprintf(buf_imu,
            "imu_size %ld sumdt %.1fms dP %3.2f %.2f %3.2f dQ %3.2f %3.2f "
            "%3.2f %3.2f dV %3.2f %3.2f %3.2f",
            pre_integrations->acc_buf.size(), pre_integrations->sum_dt * 1000,
            pre_integrations->delta_p.x(), pre_integrations->delta_p.y(),
            pre_integrations->delta_p.z(), pre_integrations->delta_q.w(),
            pre_integrations->delta_q.x(), pre_integrations->delta_q.y(),
            pre_integrations->delta_q.z(), pre_integrations->delta_v.x(),
            pre_integrations->delta_v.y(), pre_integrations->delta_v.z());
  }
  sprintf(buf,
          "VINSFrame %ld@%d stamp: %.3fs Odom: %s\nBa %.4f %.4f %.4f Bg %.4f "
          "%.4f %.4f pre_integrations: %s\n",
          frame_id, drone_id, stamp - t0, odom.toStr().c_str(), Ba(0), Ba(1),
          Ba(2), Bg(0), Bg(1), Bg(2), buf_imu);
  return std::string(buf);
}

swarm_msgs::VIOFrame VINSFrame::toROS() {
  swarm_msgs::VIOFrame msg;
  msg.header.stamp = ros::Time(stamp);
  msg.header.frame_id = "world";
  msg.frame_id = frame_id;
  msg.drone_id = drone_id;
  msg.is_keyframe = is_keyframe;
  msg.reference_frame_id = reference_frame_id;
  msg.odom = odom.toRos();
  return msg;
}

swarm_msgs::VIOFrame VINSFrame::toROS(const std::vector<Swarm::Pose>& exts) {
  swarm_msgs::VIOFrame msg;
  msg.header.stamp = ros::Time(stamp);
  msg.header.frame_id = "world";
  msg.frame_id = frame_id;
  msg.drone_id = drone_id;
  msg.is_keyframe = is_keyframe;
  msg.reference_frame_id = reference_frame_id;
  msg.odom = odom.toRos();
  for (unsigned int i = 0; i < exts.size(); i++) {
    msg.extrinsics.emplace_back(exts[i].toROS());
  }
  return msg;
}

void VINSFrame::toVector(const StatePtr& pose, const StatePtr& spd_bias) const {
  odom.pose().to_vector(pose);
  auto spd_bias_ptr = CheckGetPtr(spd_bias);
  spd_bias_ptr[0] = odom.vel().x();
  spd_bias_ptr[1] = odom.vel().y();
  spd_bias_ptr[2] = odom.vel().z();

  spd_bias_ptr[3] = Ba.x();
  spd_bias_ptr[4] = Ba.y();
  spd_bias_ptr[5] = Ba.z();

  spd_bias_ptr[6] = Bg.x();
  spd_bias_ptr[7] = Bg.y();
  spd_bias_ptr[8] = Bg.z();
}

void VINSFrame::fromVector(const StatePtr& pose, const StatePtr& spd_bias) {
  odom.pose().from_vector(pose);
  auto spd_bias_ptr = CheckGetPtr(spd_bias);
  odom.vel().x() = spd_bias_ptr[0];
  odom.vel().y() = spd_bias_ptr[1];
  odom.vel().z() = spd_bias_ptr[2];

  Ba.x() = spd_bias_ptr[3];
  Ba.y() = spd_bias_ptr[4];
  Ba.z() = spd_bias_ptr[5];

  Bg.x() = spd_bias_ptr[6];
  Bg.y() = spd_bias_ptr[7];
  Bg.z() = spd_bias_ptr[8];
}
}  // namespace D2Common