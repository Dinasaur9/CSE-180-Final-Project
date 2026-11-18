#include <cmath>
#include <memory>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"


// Robot's current pose in odom
double robot_x = 0.0;
double robot_y = 0.0;
double robot_yaw = 0.0;

// Starting pose
double start_x = 0.0;
double start_y = 0.0;
double start_yaw = 0.0;

// Make sure the starting pose is only saved once
bool start_saved = false;

// Current laser scan
std::vector<float> scan_ranges;
double scan_range_min = 0.0;
double scan_range_max = 0.0;

// Whether an obstacle is currently too close
bool obstacle_detected = false;

// Current waypoint
int current_waypoint = 0;

// Personal Lab 4 values
const double L = 1.20;
const double D_STOP = 0.56;

// Waypoints in the start frame S
double waypoints_s[4][2] =
{
    {L, 0.0},
    {L, L},
    {0.0, L},
    {0.0, 0.0}
};


// Convert a point from start frame S to odom
void transform_s_to_odom(
    double x_s,
    double y_s,
    double &x_odom,
    double &y_odom)
{
    x_odom = start_x
           + std::cos(start_yaw) * x_s
           - std::sin(start_yaw) * y_s;

    y_odom = start_y
           + std::sin(start_yaw) * x_s
           + std::cos(start_yaw) * y_s;
}


// Odometry callback
void odom_callback(
    const nav_msgs::msg::Odometry::SharedPtr msg)
{
    robot_x = msg->pose.pose.position.x;
    robot_y = msg->pose.pose.position.y;

    double z = msg->pose.pose.orientation.z;
    double w = msg->pose.pose.orientation.w;

    robot_yaw = 2.0 * std::atan2(z, w);

    // Save the starting pose only once
    if (!start_saved)
    {
        start_x = robot_x;
        start_y = robot_y;
        start_yaw = robot_yaw;

        start_saved = true;

        RCLCPP_INFO(
            rclcpp::get_logger("patrol"),
            "Start pose: x=%.3f y=%.3f yaw=%.3f",
            start_x,
            start_y,
            start_yaw);

        // Print transformed W1 and W3
        double w1_x, w1_y;
        double w3_x, w3_y;

        transform_s_to_odom(
            waypoints_s[0][0],
            waypoints_s[0][1],
            w1_x,
            w1_y);

        transform_s_to_odom(
            waypoints_s[2][0],
            waypoints_s[2][1],
            w3_x,
            w3_y);

        RCLCPP_INFO(
            rclcpp::get_logger("patrol"),
            "W1 in odom: x=%.3f y=%.3f",
            w1_x,
            w1_y);

        RCLCPP_INFO(
            rclcpp::get_logger("patrol"),
            "W3 in odom: x=%.3f y=%.3f",
            w3_x,
            w3_y);
    }
}


// Laser scanner callback
void scan_callback(
    const sensor_msgs::msg::LaserScan::SharedPtr msg)
{
    scan_ranges = msg->ranges;
    scan_range_min = msg->range_min;
    scan_range_max = msg->range_max;

    obstacle_detected = false;

    if (scan_ranges.empty())
    {
        return;
    }

    // Check the center portion of the scan.
    // The center corresponds approximately to straight ahead.
    int center = scan_ranges.size() / 2;

    // Check a small region around the center.
    int check_width = 10;

    for (int i = center - check_width;
         i <= center + check_width;
         i++)
    {
        if (i < 0 || i >= static_cast<int>(scan_ranges.size()))
        {
            continue;
        }

        float range = scan_ranges[i];

        // Ignore invalid readings
        if (!std::isfinite(range))
        {
            continue;
        }

        if (range >= scan_range_min &&
            range <= scan_range_max &&
            range < D_STOP)
        {
            obstacle_detected = true;
            break;
        }
    }
}


// Main control timer
void control_callback(
    const rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher)
{
    geometry_msgs::msg::Twist cmd;

    // Do nothing until the initial pose has been saved
    if (!start_saved)
    {
        publisher->publish(cmd);
        return;
    }

    // SAFETY STOP
    if (obstacle_detected)
    {
        cmd.linear.x = 0.0;
        cmd.angular.z = 0.0;

        publisher->publish(cmd);

        RCLCPP_INFO(
            rclcpp::get_logger("patrol"),
            "Pausing: obstacle detected within %.2f m",
            D_STOP);

        return;
    }

    // Get current waypoint in odom
    double goal_x;
    double goal_y;

    transform_s_to_odom(
        waypoints_s[current_waypoint][0],
        waypoints_s[current_waypoint][1],
        goal_x,
        goal_y);

    // Difference between goal and robot in odom
    double dx = goal_x - robot_x;
    double dy = goal_y - robot_y;

    // Transform difference into robot frame R
    double x_r =
        std::cos(robot_yaw) * dx +
        std::sin(robot_yaw) * dy;

    double y_r =
        -std::sin(robot_yaw) * dx +
        std::cos(robot_yaw) * dy;

    // Distance to waypoint
    double distance =
        std::sqrt(x_r * x_r + y_r * y_r);

    // Check whether waypoint has been reached
    if (distance < 0.10)
    {
        RCLCPP_INFO(
            rclcpp::get_logger("patrol"),
            "Reached W%d",
            current_waypoint + 1);

        current_waypoint++;

        if (current_waypoint >= 4)
        {
            current_waypoint = 0;
        }

        publisher->publish(cmd);
        return;
    }

    // Angle from robot to waypoint
    double angle = std::atan2(y_r, x_r);

    // Simple proportional controller
    cmd.linear.x = 0.20;
    cmd.angular.z = 1.5 * angle;

    // Limit angular velocity
    if (cmd.angular.z > 1.0)
    {
        cmd.angular.z = 1.0;
    }

    if (cmd.angular.z < -1.0)
    {
        cmd.angular.z = -1.0;
    }

    publisher->publish(cmd);
}


int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    auto node = rclcpp::Node::make_shared("patrol");

    auto publisher =
        node->create_publisher<geometry_msgs::msg::Twist>(
            "/cmd_vel",
            10);

    auto odom_subscription =
        node->create_subscription<nav_msgs::msg::Odometry>(
            "/odom",
            10,
            odom_callback);

    auto scan_subscription =
        node->create_subscription<sensor_msgs::msg::LaserScan>(
            "/scan",
            10,
            scan_callback);

    auto timer =
        node->create_wall_timer(
            std::chrono::milliseconds(100),
            [publisher]()
            {
                control_callback(publisher);
            });

    RCLCPP_INFO(
        node->get_logger(),
        "Patrol starting: L=%.2f m, D_stop=%.2f m",
        L,
        D_STOP);

    rclcpp::spin(node);

    rclcpp::shutdown();

    return 0;
}