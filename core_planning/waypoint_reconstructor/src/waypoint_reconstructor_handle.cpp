/*
    Formula Student Driverless Project (FSD-Project).
    Copyright (c) 2019:
     - chentairan <killasipilin@gmail.com>

    FSD-Project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FSD-Project is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FSD-Project.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <ros/ros.h>
#include "waypoint_reconstructor_handle.hpp"
#include "register.h"
#include <chrono>

namespace ns_waypoint_reconstructor {

// Constructor
Waypoint_ReconstructorHandle::Waypoint_ReconstructorHandle(ros::NodeHandle &nodeHandle) :
    nodeHandle_(nodeHandle),
    private_nh_("~"),
    waypoint_reconstructor_(nodeHandle) {
  ROS_INFO("Constructing Handle");
  v2vFlag=poseFlag=false;
  loadParameters();
  subscribeToTopics();
  publishToTopics();
}

// Getters
int Waypoint_ReconstructorHandle::getNodeRate() const { return node_rate_; }

// Methods
void Waypoint_ReconstructorHandle::loadParameters() {
  ROS_INFO("loading handle parameters");
  if (!private_nh_.param<std::string>("localization_utm_topic_name",current_pose_topic_name_,"/localization/utmpose")) {
    ROS_WARN_STREAM("Did not load localization_utm_topic_name. Standard value is: " << current_pose_topic_name_);
  }
  if (!private_nh_.param<std::string>("v2v_topic_name",v2v_topic_name_,"v2v")) {
    ROS_WARN_STREAM("Did not load v2v_topic_name_. Standard value is: " << v2v_topic_name_);
  }
  if (!private_nh_.param<std::string>("final_waypoints_topic_name",final_waypoint_topic_name_,"final_waypoints")) {
    ROS_WARN_STREAM("Did not load final_waypoints_topic_name. Standard value is: " << final_waypoint_topic_name_);
  }
  if (!private_nh_.param<std::string>("pose_topic_name",pose_topic_name_,"current_pose")) {
    ROS_WARN_STREAM("Did not load pose_topic_name. Standard value is: " << pose_topic_name_);
  }
  if (!private_nh_.param<std::string>("velocity_topic_name",velocity_topic_name_,"current_velocity")) {
    ROS_WARN_STREAM("Did not load velocity_topic_name. Standard value is: " << velocity_topic_name_);
  }
  if (!private_nh_.param("node_rate", node_rate_, 1)) {
    ROS_WARN_STREAM("Did not load node_rate. Standard value is: " << node_rate_);
  }

  nodeHandle_.param<double>("Gps_origin/x",origin_.x,274083.3651381);
  nodeHandle_.param<double>("Gps_origin/y",origin_.y,4006502.509805);
  nodeHandle_.param<double>("Gps_origin/z",origin_.z,74.80);
  ROS_INFO_STREAM("Gps origin: x: "<<origin_.x<<", y: " << origin_.y<<", z: "<< origin_.z);
  waypoint_reconstructor_.setGpsOrigin(origin_);
}

void Waypoint_ReconstructorHandle::subscribeToTopics() {
  ROS_INFO("subscribe to topics");
  sub1_ = nodeHandle_.subscribe(current_pose_topic_name_, 10, &Waypoint_ReconstructorHandle::poseCallback, this);
  sub2_ = nodeHandle_.subscribe(v2v_topic_name_, 10, &Waypoint_ReconstructorHandle::V2VCallback, this);
}//FIXME: change topic name you want to subscribe.

void Waypoint_ReconstructorHandle::publishToTopics() {
  ROS_INFO("publish to topics");
  pub1_ = nodeHandle_.advertise<autoware_msgs::Lane>(final_waypoint_topic_name_, 10);
  pub2_ = nodeHandle_.advertise<geometry_msgs::PoseStamped>(pose_topic_name_, 10);
}//FIXME: change topic name you want to advertise.

void Waypoint_ReconstructorHandle::run() {
  std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
  //check read ready
  if(!v2vFlag || !poseFlag){
      ROS_WARN("Necessary topics are not subscribed yet ... ");
      ROS_WARN("v2v:%d  pose:%d",v2vFlag,poseFlag);
      return;
  }
  //
  waypoint_reconstructor_.runAlgorithm();
  std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
  double time_round = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
  std::cout << "time cost = " << time_round << ", frequency = " << 1 / time_round << std::endl;
  sendMsg();
  v2vFlag=poseFlag=false;
}

void Waypoint_ReconstructorHandle::sendMsg() {
  if(waypoint_reconstructor_.isInited()){
     pub1_.publish(waypoint_reconstructor_.getFinalWaypoints());
  }
  pub2_.publish(waypoint_reconstructor_.getCurrentPose());
}//FIXME: change the msg name you want to publish.

void Waypoint_ReconstructorHandle::poseCallback(const nav_msgs::OdometryConstPtr &msg) {
  waypoint_reconstructor_.setSelfPose(msg);
  poseFlag=true;
}//FIXME: change or add callback functions to receive msgs.

void Waypoint_ReconstructorHandle::V2VCallback(const common_msgs::V2VConstPtr &msg) {
  waypoint_reconstructor_.setV2V(msg);
  v2vFlag=true;
}//FIXME: change or add callback functions to receive msgs.

}