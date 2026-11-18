#include <chrono>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"

using namespace std::chrono_literals;

// N = 8
const double v_max = 0.26;
const double w_max = 0.80;
const double T = 0.90;

// Store the most recent command
geometry_msgs::msg::Twist last_command;

// Store when the last command arrived
std::chrono::steady_clock::time_point last_time;

// Publisher
rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher;

// Used to print messages only when the state changes
bool driving = false;


// Keep a value between a minimum and maximum
double clamp(double value, double minimum, double maximum)
{
    if (value > maximum)
        return maximum;

    if (value < minimum)
        return minimum;

    return value;
}


// Called whenever a new teleop command arrives
void command_callback(
    const geometry_msgs::msg::Twist::SharedPtr msg)
{
    // Save the command
    last_command = *msg;

    // Limit forward/backward speed
    last_command.linear.x =
        clamp(last_command.linear.x, -v_max, v_max);

    // Limit rotation speed
    last_command.angular.z =
        clamp(last_command.angular.z, -w_max, w_max);

    // Remember when the command arrived
    last_time = std::chrono::steady_clock::now();

    // Print when we start driving
    if (!driving)
    {
        RCLCPP_INFO(
            rclcpp::get_logger("safedrive"),
            "Driving"
        );

        driving = true;
    }
}


// Called 10 times per second
void timer_callback()
{
    auto current_time = std::chrono::steady_clock::now();

    double elapsed =
        std::chrono::duration<double>(current_time - last_time).count();

    if (elapsed <= T)
    {
        // Keep sending the most recent command
        publisher->publish(last_command);
    }
    else
    {
        // Stop the robot
        geometry_msgs::msg::Twist stop_command;

        publisher->publish(stop_command);

        // Only print once when we change from driving to stopped
        if (driving)
        {
            RCLCPP_INFO(
                rclcpp::get_logger("safedrive"),
                "Stopped - no command received"
            );

            driving = false;
        }
    }
}


int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    auto node =
        rclcpp::Node::make_shared("safedrive");

    // Publish commands to the TurtleBot
    publisher =
        node->create_publisher<geometry_msgs::msg::Twist>(
            "/cmd_vel",
            10
        );

    // Receive commands from turtle_teleop_key
    auto subscriber =
        node->create_subscription<geometry_msgs::msg::Twist>(
            "/turtle1/cmd_vel",
            10,
            command_callback
        );

    // Start with the timer
    last_time = std::chrono::steady_clock::now();

    // 10 Hz timer = every 100 milliseconds
    auto timer =
        node->create_wall_timer(
            100ms,
            timer_callback
        );

    rclcpp::spin(node);

    rclcpp::shutdown();

    return 0;
}