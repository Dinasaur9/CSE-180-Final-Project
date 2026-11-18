#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <geometry_msgs/msg/twist.hpp>

#include <cmath>

// Variables that need to be remembered between callbacks
bool start_recorded = false;
bool goal_reached = false;

double start_x;
double start_y;
double start_theta;

double goal_x_odom;
double goal_y_odom;

rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr velocity_publisher;


// Odometry callback
void odometry_callback(const nav_msgs::msg::Odometry::SharedPtr msg)
{
    // Get the robot's current position
    double x = msg->pose.pose.position.x;
    double y = msg->pose.pose.position.y;

    // Get the quaternion
    double qx = msg->pose.pose.orientation.x;
    double qy = msg->pose.pose.orientation.y;
    double qz = msg->pose.pose.orientation.z;
    double qw = msg->pose.pose.orientation.w;

    // Convert quaternion to yaw angle
    double theta = atan2(
        2.0 * (qw * qz + qx * qy),
        1.0 - 2.0 * (qy * qy + qz * qz)
    );

    // Record the starting pose on the first odometry message
    if (!start_recorded)
    {
        start_x = x;
        start_y = y;
        start_theta = theta;

        // Goal in the start frame S
        double a = 1.00;
        double b = 0.50;

        // Transform goal from S into odom
        goal_x_odom =
            start_x
            + cos(start_theta) * a
            - sin(start_theta) * b;

        goal_y_odom =
            start_y
            + sin(start_theta) * a
            + cos(start_theta) * b;

        RCLCPP_INFO(
            rclcpp::get_logger("gotogoal"),
            "Goal in odom: x = %.6f, y = %.6f",
            goal_x_odom,
            goal_y_odom
        );

        start_recorded = true;
    }

    // Difference between goal and current robot position in odom
    double delta_x = goal_x_odom - x;
    double delta_y = goal_y_odom - y;

    // Transform the goal from odom into the current robot frame R
    double x_R =
        cos(theta) * delta_x
        + sin(theta) * delta_y;

    double y_R =
        -sin(theta) * delta_x
        + cos(theta) * delta_y;

    // Calculate distance and bearing
    double d = sqrt(x_R * x_R + y_R * y_R);
    double alpha = atan2(y_R, x_R);

    // Create velocity command
    auto cmd = geometry_msgs::msg::Twist();

    // Stop when the robot reaches the goal
    if (d < 0.10)
    {
        cmd.linear.x = 0.0;
        cmd.angular.z = 0.0;

        if (!goal_reached)
        {
            RCLCPP_INFO(
                rclcpp::get_logger("gotogoal"),
                "Goal reached!"
            );

            goal_reached = true;
        }
    }
    else
    {
        // Angular velocity = alpha, limited to +/- 0.8 rad/s
        cmd.angular.z = alpha;

        if (cmd.angular.z > 0.8)
        {
            cmd.angular.z = 0.8;
        }

        if (cmd.angular.z < -0.8)
        {
            cmd.angular.z = -0.8;
        }

        // Only drive forward when roughly facing the goal
        if (fabs(alpha) < 0.3)
        {
            cmd.linear.x = 0.5 * d;

            // Limit linear velocity to 0.2 m/s
            if (cmd.linear.x > 0.2)
            {
                cmd.linear.x = 0.2;
            }
        }
        else
        {
            cmd.linear.x = 0.0;
        }
    }

    // Publish the velocity command
    velocity_publisher->publish(cmd);
}


int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    auto node = rclcpp::Node::make_shared("gotogoal");

    // Publisher for TurtleBot velocity commands
    velocity_publisher =
        node->create_publisher<geometry_msgs::msg::Twist>(
            "/cmd_vel",
            10
        );

    // Subscriber for odometry
    auto odometry_subscriber =
        node->create_subscription<nav_msgs::msg::Odometry>(
            "/odom",
            10,
            odometry_callback
        );

    RCLCPP_INFO(
        node->get_logger(),
        "Go-to-goal node started."
    );

    rclcpp::spin(node);

    rclcpp::shutdown();

    return 0;
}