#include "registration_apps/app_ros.hpp"
#include "registration_apps/yaml_configurator.hpp"
#include "aicp_common_utils/common.hpp"

using namespace std;

int main(int argc, char** argv){
    ros::init(argc, argv, "aicp_ros_online_node");
    ros::NodeHandle nh("~");

    CommandLineConfig cl_cfg;
    cl_cfg.config_file.append(CONFIG_LOC);
    cl_cfg.config_file.append(PATH_SEPARATOR);
    cl_cfg.config_file.append("aicp_config.yaml");
    cl_cfg.working_mode = "robot"; // e.g. robot - POSE_BODY has been already corrected
                                   // or debug - apply previous transforms to POSE_BODY
    cl_cfg.failure_prediction_mode = FALSE; // compute Alignment Risk
    cl_cfg.reference_update_frequency = 5;

    cl_cfg.pose_body_channel = "/state_estimator/pose_in_odom";
    cl_cfg.output_channel = "/aicp/pose_corrected"; // Create new channel...
    cl_cfg.verbose = FALSE; // enable visualization for debug

    aicp::VelodyneAccumulatorConfig va_cfg;
    va_cfg.batch_size = 80; // 240 is about 1 sweep at 5RPM // 80 is about 1 sweep at 15RPM
    va_cfg.min_range = 0.50; // 1.85; // remove all the short range points
    va_cfg.max_range = 15.0; // we can set up to 30 meters (guaranteed range)
    va_cfg.lidar_topic ="/velodyne/point_cloud_filtered";
    va_cfg.inertial_frame = "/odom";

    nh.getParam("config_file", cl_cfg.config_file);
    nh.getParam("working_mode", cl_cfg.working_mode);
    nh.getParam("failure_prediction_mode", cl_cfg.failure_prediction_mode);
    nh.getParam("reference_update_frequency", cl_cfg.reference_update_frequency);

    nh.getParam("pose_body_channel", cl_cfg.pose_body_channel);
    nh.getParam("output_channel", cl_cfg.output_channel);
    nh.getParam("verbose", cl_cfg.verbose);

    nh.getParam("batch_size", va_cfg.batch_size);
    nh.getParam("min_range", va_cfg.min_range);
    nh.getParam("max_range", va_cfg.max_range);
    nh.getParam("lidar_channel", va_cfg.lidar_topic);
    nh.getParam("inertial_frame", va_cfg.inertial_frame);

    std::string bot_param_path = "";
    nh.getParam("bot_param_path", bot_param_path);

    /*===================================
    =            YAML Config            =
    ===================================*/
    aicp::YAMLConfigurator yaml_conf;
    if(!yaml_conf.parse(cl_cfg.config_file)){
        cerr << "ERROR: could not parse file " << cl_cfg.config_file << endl;
        return -1;
    }
    yaml_conf.printParams();

    /*===================================
    =              Start App            =
    ===================================*/
    std::shared_ptr<aicp::AppROS> app(new aicp::AppROS(nh,
                                                       cl_cfg,
                                                       va_cfg,
                                                       yaml_conf.getRegistrationParams(),
                                                       yaml_conf.getOverlapParams(),
                                                       yaml_conf.getClassificationParams(),
                                                       bot_param_path));

    // Subscribers
    ros::Subscriber lidar_sub = nh.subscribe(va_cfg.lidar_topic, 100, &aicp::AppROS::velodyneCallBack, app.get());
    ros::Subscriber pose_sub = nh.subscribe(cl_cfg.pose_body_channel, 100, &aicp::AppROS::robotPoseCallBack, app.get());

    app->run();
    ros::spin();

    return 0;
}
