// CSE180 Final Project -- search node (navigation part)
//
// Visits a hardcoded list of map-frame waypoints with the MRTP Navigator,
// turns in place at each one (Spin) so the camera can look around, and then
// returns to the starting pose.
//
// The perception part (detections + depth + camera_info + TF2 -> people on
// the map) will be added later; see the "PERCEPTION HOOK" comments.
//
// Run with:  ros2 run search search --ros-args -p use_sim_time:=true

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "navigation/navigation.hpp"


// ---------------------------------------------------------------------------
// Waypoints (map frame == Gazebo world frame)
// ---------------------------------------------------------------------------
struct Waypoint
{
  std::string name;
  double x;
  double y;
  bool spin;     // turn in place here to look around?
};

// See tabel_tips.txt for how these were picked.
const std::vector<Waypoint> WAYPOINTS =
{
  {"P1 back left",                    -12.30,  4.00, true},
  {"P2 back right",                   -12.30, -5.30, true},
  {"P3 back middle, before red crates", -0.72, 0.58, true},
  {"P4 between aisles red/brown",       4.00,  3.36, true},
  {"P5 between aisles brown/blue",      8.55,  3.40, true},
  {"P6 behind blue",                   11.16,  3.40, true},
  {"P7 stock pile next to blue",        9.95, -3.70, true},
  {"P8 middle of stacks",               4.21, -3.70, true},
  {"P9 stacks close to red",            1.60, -3.70, true},
};

// A full look-around is split into several partial spins. The short pause
// between them gives the detector a few steady frames in each direction.
const int SPIN_STEPS = 4;
const double SPIN_STEP_ANGLE = 2.0 * M_PI / SPIN_STEPS;


// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
geometry_msgs::msg::Pose::SharedPtr make_pose(double x, double y, double yaw)
{
  auto pose = std::make_shared<geometry_msgs::msg::Pose>();
  pose->position.x = x;
  pose->position.y = y;
  pose->position.z = 0.0;
  // yaw-only quaternion
  pose->orientation.z = std::sin(yaw / 2.0);
  pose->orientation.w = std::cos(yaw / 2.0);
  return pose;
}

// Block until the current Navigator task ends. IsTaskComplete() spins the
// navigator node internally, so this loop does not starve its callbacks.
bool wait_for_task(Navigator & navigator)
{
  while (rclcpp::ok() && !navigator.IsTaskComplete()) {
    // PERCEPTION HOOK: the perception node runs on its own executor thread,
    // so nothing needs to happen here.
  }
  return navigator.GetResult() == rclcpp_action::ResultCode::SUCCEEDED;
}

// Drive to (x, y, yaw). On failure, clear the costmaps (stale obstacles are
// the usual cause) and retry once.
bool go_to(Navigator & navigator, double x, double y, double yaw,
           const std::string & name)
{
  for (int attempt = 1; attempt <= 2 && rclcpp::ok(); attempt++) {
    RCLCPP_INFO(navigator.get_logger(),
                "Going to %s (%.2f, %.2f), attempt %d",
                name.c_str(), x, y, attempt);

    if (navigator.GoToPose(make_pose(x, y, yaw)) && wait_for_task(navigator)) {
      RCLCPP_INFO(navigator.get_logger(), "Reached %s", name.c_str());
      return true;
    }

    RCLCPP_WARN(navigator.get_logger(),
                "Failed to reach %s; clearing costmaps", name.c_str());
    navigator.ClearAllCostmaps();
  }
  return false;
}

// Turn a full circle in place, in SPIN_STEPS pieces.
void look_around(Navigator & navigator, const std::string & name)
{
  RCLCPP_INFO(navigator.get_logger(), "Looking around at %s", name.c_str());
  for (int i = 0; i < SPIN_STEPS && rclcpp::ok(); i++) {
    if (!navigator.Spin(SPIN_STEP_ANGLE) || !wait_for_task(navigator)) {
      // Spin aborts if it would hit something (e.g. next to a shelf).
      // Skip the rest of this turn rather than getting stuck.
      RCLCPP_WARN(navigator.get_logger(),
                  "Spin %d/%d failed at %s, skipping the rest",
                  i + 1, SPIN_STEPS, name.c_str());
      return;
    }
  }
}


// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto navigator = std::make_shared<Navigator>(true, false);

  // All nodes must run on simulation time.
  if (!navigator->get_parameter("use_sim_time").as_bool()) {
    navigator->set_parameter(rclcpp::Parameter("use_sim_time", true));
  }

  // Starting pose in the MAP frame. Defaults = AMCL initial_pose in
  // nav2_params_tb4.yaml (used by tb4_simulation.launch.py, depot world).
  // WaitUntilNav2Active() re-publishes this to /initialpose, so it MUST match
  // where the robot actually is, otherwise AMCL gets reset to the wrong place.
  const double start_x = navigator->declare_parameter("start_x", 0.0);
  const double start_y = navigator->declare_parameter("start_y", 0.0);
  const double start_yaw = navigator->declare_parameter("start_yaw", 0.0);

  // The waypoints are in Gazebo WORLD coordinates, but Nav2 wants MAP
  // coordinates. The robot spawns at world (-8, 0) (tb4_simulation.launch.py)
  // and that spot is map (0, 0), so map = world - spawn + start.
  const double spawn_world_x = navigator->declare_parameter("spawn_world_x", -8.0);
  const double spawn_world_y = navigator->declare_parameter("spawn_world_y", 0.0);
  const double world_to_map_x = start_x - spawn_world_x;
  const double world_to_map_y = start_y - spawn_world_y;

  navigator->SetInitialPose(make_pose(start_x, start_y, start_yaw));
  navigator->WaitUntilNav2Active();

  RCLCPP_INFO(navigator->get_logger(),
              "Nav2 active. Start pose (%.2f, %.2f, %.2f). %zu waypoints.",
              start_x, start_y, start_yaw, WAYPOINTS.size());

  // PERCEPTION HOOK: create the perception node here and spin it in a
  // background thread (its own executor), e.g.
  //   auto searcher = std::make_shared<PersonSearch>();
  //   rclcpp::executors::SingleThreadedExecutor exec;
  //   exec.add_node(searcher);
  //   std::thread spin_thread([&exec]() { exec.spin(); });

  // Visit the waypoints in order
  double prev_x = start_x;
  double prev_y = start_y;
  int reached = 0;

  for (size_t i = 0; i < WAYPOINTS.size() && rclcpp::ok(); i++) {
    const Waypoint & wp = WAYPOINTS[i];
    const double x = wp.x + world_to_map_x;
    const double y = wp.y + world_to_map_y;

    // Arrive facing the direction of travel, so the camera already looks
    // into the new area on the way in.
    const double yaw = std::atan2(y - prev_y, x - prev_x);

    if (go_to(*navigator, x, y, yaw, wp.name)) {
      reached++;
      if (wp.spin) {
        look_around(*navigator, wp.name);
      }
      prev_x = x;
      prev_y = y;
    } else {
      // Move on; the next waypoint is planned from wherever we are.
      RCLCPP_ERROR(navigator->get_logger(),
                   "Skipping %s after repeated failures", wp.name.c_str());
    }
  }

  // Return to the starting pose
  go_to(*navigator, start_x, start_y, start_yaw, "start pose");

  RCLCPP_INFO(navigator->get_logger(),
              "Search finished: reached %d of %zu waypoints.",
              reached, WAYPOINTS.size());

  // PERCEPTION HOOK: stop the executor thread and print the final list
  // (map coordinates + number of detections for each person).

  rclcpp::shutdown();
  return 0;
}
