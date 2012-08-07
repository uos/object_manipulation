#include <iostream>
#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>
#include <std_msgs/String.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include <boost/bind.hpp>
#include <boost/thread.hpp>

//PCL specific includes
#include <pcl/compression/octree_pointcloud_compression.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/ros/conversions.h>

#include <compressed_pointcloud_transport/CompressedPointCloud.h>

// **** 10 ***** 20 ****** 30 ****** 40 ****** 50 ****** 60 ****** 70 ****** 80 ****** 90 ***** 100 ***** 110 ***** 120

typedef pcl::PointXYZRGB PointT;


class Decompressor{

protected:
  ros::NodeHandle nh_, pnh_;
  ros::Publisher pub_;
  boost::shared_ptr<ros::Subscriber> sub_;

  pcl::octree::PointCloudCompression<PointT> decoder_;
  ros::Timer subscriber_timer_;

public:
  Decompressor()
    :
    nh_()
    , pnh_("~")
    // ,pcl_cloud(new pcl::PointCloud<PointT>)
  {
    // Create a ROS publisher for the output point cloud
    pub_ = nh_.advertise<sensor_msgs::PointCloud2>("output", 1);

    // Create a ROS subscriber for the input point cloud
    sub_.reset(new ros::Subscriber());
    *sub_ = nh_.subscribe("input", 2, &Decompressor::cloud_cb, this);

    subscriber_timer_ = nh_.createTimer(ros::Duration(1.0), boost::bind(&Decompressor::timerCB, this));

  }

protected:
  //void cloud_cb (const sensor_msgs::PointCloud2ConstPtr& input) {
  void cloud_cb (const compressed_pointcloud_transport::CompressedPointCloud::ConstPtr& input) {

    if(!pub_.getNumSubscribers())
    {
      ROS_INFO("Got a cloud!");

      // clouds
      pcl::PointCloud<PointT>::Ptr pcl_cloud(new pcl::PointCloud<PointT>);
      sensor_msgs::PointCloud2::Ptr ros_cloud(new sensor_msgs::PointCloud2);

      // Stringstream to retrieve compressed point cloud
      std::stringstream compressed_data(input->data);

      //Decompress the point cloud
      decoder_.decodePointCloud(compressed_data, pcl_cloud);

      //Convert back to sensor_msgs::PointCloud2
      pcl::toROSMsg(*pcl_cloud, *ros_cloud);

      // restore the cloud header
      ros_cloud->header = input->header;

      ROS_DEBUG_NAMED("decompressor", "Uncompressed cloud with frame [%s] and stamp [%f]", pcl_cloud->header.frame_id.c_str(), pcl_cloud->header.stamp.toSec());

      pub_.publish(ros_cloud);
      int output_size = sizeof(ros_cloud->data);
      int compressed_size = sizeof(input->data);
      ROS_DEBUG_NAMED("decompressor", "Published cloud, input size %d bytes, inflated size %d bytes.", compressed_size, output_size);
    }
    else
      ROS_DEBUG_NAMED("decompressor" ,"Received compressed cloud but there are no subscribers; not publishing.");
  }

  void timerCB()
  {
    if(pub_.getNumSubscribers())
    {
      if( !sub_ )
      {
        ROS_INFO("Making a new subscriber!");
        sub_.reset(new ros::Subscriber());
        *sub_ = nh_.subscribe("input", 2, &Decompressor::cloud_cb, this);
      }
    }
    else
    {
      if( sub_)
      {
        ROS_INFO("Deleting subscriber!");
        sub_.reset();
      }
    }
  }
};

int main(int argc, char** argv) {
  // Initialize ROS
  ros::init(argc, argv, "cloud_decompressor");
  Decompressor decomp;

  ros::Duration(1.0).sleep();
  ros::spin();
}
