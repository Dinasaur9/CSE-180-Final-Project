
# Step 3 — Warehouse Exploration

**Course:** CSE 180 — Introduction to Robotics  
**Project:** Search the Warehouse  
**Simulation:** `tb4_warehouse.launch.py`

## 1. RGB Image, Depth Image, and Camera Calibration

We inspected the RGB-D camera in the warehouse simulation and identified the following topics and message properties.

| Information | ROS2 Topic | Message Type | Resolution | Encoding |
|---|---|---|---|---|
| RGB color image | `/rgbd_camera/image` | `sensor_msgs/msg/Image` | 320 × 240 | `rgb8` |
| Depth image | `/rgbd_camera/depth_image` | `sensor_msgs/msg/Image` | 320 × 240 | `32FC1` |
| Camera calibration | `/rgbd_camera/camera_info` | `sensor_msgs/msg/CameraInfo` | 320 × 240 | N/A |

All three messages use the coordinate frame `oakd_rgb_camera_optical_frame`.

The RGB image uses `rgb8` encoding, meaning each pixel contains three 8-bit color channels (red, green, and blue). This image provides the visual information needed for person detection.

The depth image uses `32FC1` encoding, meaning each pixel contains one 32-bit floating-point depth value. Valid depth measurements are conventionally expressed in meters.

Both images have a resolution of 320 × 240 pixels. The matching dimensions allow us to associate detected image regions with depth measurements, provided the images are properly aligned.

The camera calibration topic provides the intrinsic parameters needed to convert pixel coordinates into 3D positions.

## 2. Camera Intrinsics and Pinhole Camera Model

We inspected `/rgbd_camera/camera_info` in the warehouse simulation and obtained the following camera intrinsic matrix:

$$
K =
\begin{bmatrix}
221.765 & 0 & 160 \\
0 & 221.765 & 120 \\
0 & 0 & 1
\end{bmatrix}
$$

### Intrinsic Parameters

| Parameter | Entry in `k` | Value | Description |
|---|---|---|---|
| $f_x$ | `k[0]` | 221.765 | Horizontal focal length in pixels |
| $f_y$ | `k[4]` | 221.765 | Vertical focal length in pixels |
| $c_x$ | `k[2]` | 160 | Horizontal principal point |
| $c_y$ | `k[5]` | 120 | Vertical principal point |

### Pinhole Camera Model

The pinhole camera model relates a 3D point $(X,Y,Z)$ in the camera optical frame to its pixel coordinates $(u,v)$:

$$
u = f_x\frac{X}{Z} + c_x
$$

$$
v = f_y\frac{Y}{Z} + c_y
$$

By rearranging these equations and using the optical-axis depth $d=Z$, we obtain the 3D coordinates:

$$
\boxed{X = \frac{(u-c_x)d}{f_x}}
$$

$$
\boxed{Y = \frac{(v-c_y)d}{f_y}}
$$

$$
\boxed{Z = d}
$$

### Example

Suppose a person is detected at pixel $(200,140)$ and the depth image reports a valid depth of 3.0 meters.

Using the measured camera intrinsics:

$$
X = \frac{(200-160)(3)}{221.765}
\approx 0.541\text{ m}
$$

$$
Y = \frac{(140-120)(3)}{221.765}
\approx 0.271\text{ m}
$$

$$
Z = 3.0\text{ m}
$$

Therefore, the 3D point in the camera optical frame is:

$$
\boxed{P_{\text{camera}} = (0.541,\ 0.271,\ 3.0)\text{ m}}
$$

### Why This Matters

Our person-detection system will use the RGB image to identify people and locate them in pixel coordinates. The depth image and camera intrinsic parameters will then allow us to estimate their 3D positions.

Finally, we will use TF2 to transform these coordinates from the camera optical frame into the warehouse's `map` frame so the robot can report where people were detected.

## 3. TF2 and Camera Optical Frame Orientation

We used `tf2_echo` to inspect the transformation between the robot's `base_link` frame and the camera's `oakd_rgb_camera_optical_frame`.

The transformation returned a translation of **(-0.060, 0.000, 0.244) meters** and a rotation of **(-90°, 0°, -90°)** in roll, pitch, and yaw. This indicates that the camera optical frame is located approximately 6 cm behind and 24.4 cm above the robot's base frame origin.

The homogeneous transformation matrix obtained was:

$$
T_{\text{base}\leftarrow\text{camera}} =
\begin{bmatrix}
0 & 0 & 1 & -0.060 \\
-1 & 0 & 0 & 0.000 \\
0 & -1 & 0 & 0.244 \\
0 & 0 & 0 & 1
\end{bmatrix}
$$

The rotation matrix was:

$$
R =
\begin{bmatrix}
0 & 0 & 1 \\
-1 & 0 & 0 \\
0 & -1 & 0
\end{bmatrix}
$$

From this matrix, we confirmed the following axis orientations:

| Axis | Robot Frame (`base_link`) | Camera Optical Frame |
|---|---|---|
| +X | Forward | Right |
| +Y | Left | Down |
| +Z | Up | Forward |

The frames use different orientations because they serve different purposes. The robot frame follows the standard ROS convention for navigation, while the camera optical frame follows the convention used in computer vision, where the X-axis corresponds to the image's horizontal direction, the Y-axis points down the image, and the Z-axis points in the camera's viewing direction.

**Connection to our project:** When our system detects a person, the RGB image and depth measurements allow us to calculate their 3D position relative to the camera. We can then use TF2 to transform that position into the robot's frame and eventually the warehouse's `map` frame. This is necessary for reporting the person's location in the warehouse rather than only their position relative to the camera.

## 4. What unit does the depth image use? What value does a pixel hold when there is nothing within range?

The depth image is published on `/rgbd_camera/depth_image`
using `sensor_msgs/msg/Image` with `32FC1` encoding.
Each pixel contains a 32-bit floating-point depth measurement,
with valid depth values expressed in **meters (m)**.

When the camera cannot obtain a finite depth measurement, a
pixel may contain positive infinity (`+inf`). We verified this
by inspecting an actual depth image from the warehouse simulation.

### Gazebo Camera Configuration

We inspected the OAK-D camera configuration in:

`/opt/ros/jazzy/share/nav2_minimal_tb4_description/urdf/sensors/oakd.urdf.xacro`

The camera's sensor definition includes:

```xml
<clip>
  <near>0.3</near>
  <far>100</far>
</clip>
```

This confirms that the simulated RGB-D camera has a configured
clipping range of **0.3 m to 100 m**.

The configuration also specifies a resolution of 320 × 240
pixels and an update rate of 10 Hz.

### Experimental Results

We subscribed to `/rgbd_camera/depth_image` and inspected
one depth frame using a Python script.

| Property | Observed Value |
|---|---|
| Resolution | 320 × 240 |
| Encoding | `32FC1` |
| Total pixels | 76,800 |
| Finite pixels | 76,401 |
| NaN pixels | 0 |
| Positive infinity pixels | 399 |
| Negative infinity pixels | 0 |
| Zero-valued pixels | 0 |
| Minimum finite depth | 0.4468 m |
| Maximum finite depth | 46.3631 m |
| Center pixel depth | 29.4214 m |

Of the 76,800 pixels, 399 contained positive infinity
(`+inf`), approximately 0.52% of the image.

The finite depth measurements were within the camera's
configured clipping range of 0.3 m to 100 m.

This confirms that `+inf` is used for at least some missing
or invalid depth measurements in our simulation. The camera
configuration does not explicitly specify the reason for
each invalid pixel.

### Verification Script

We used a ROS2 Python script to inspect the depth image It can be found at `scripts/inspect_depth.py`.

To reproduce the results, launch the warehouse simulation and run:

```bash
python3 scripts/inspect_depth.py
```

The numerical results may vary depending on the robot's
position, camera view, and simulation state.

### Connection to Our Project

Our person-detection system will use RGB images to identify
people and depth measurements to calculate their 3D positions.

Before using a depth measurement, we must verify that it is
finite, positive, and within the camera's configured sensing
range of 0.3 m to 100 m.

If a detected person's depth pixel contains `+inf` or another
invalid value, we should reject that measurement or select
another valid pixel from the person's detected region.

This will help prevent incorrect 3D coordinates from being
reported as person locations in the warehouse.

## 5. Which action servers does Nav2 offer (ros2 action list)? Read the MRTP navigation package. Which Navigator methods will you need?


We ran `ros2 action list -t` while the warehouse simulation
was running and identified 17 action servers.

| Action Server | Purpose |
|---|---|
| `/assisted_teleop` | Assisted manual control with collision avoidance |
| `/backup` | Move backward |
| `/compute_and_track_route` | Compute and follow a route |
| `/compute_path_through_poses` | Plan a path through multiple poses |
| `/compute_path_to_pose` | Plan a path to one pose |
| `/compute_route` | Compute a route through a routing graph |
| `/dock_robot` | Dock the robot |
| `/drive_on_heading` | Move along a specified heading |
| `/follow_gps_waypoints` | Follow GPS waypoints |
| `/follow_path` | Execute a planned path |
| `/follow_waypoints` | Follow a sequence of waypoints |
| `/navigate_through_poses` | Navigate through multiple poses |
| `/navigate_to_pose` | Navigate to one target pose |
| `/smooth_path` | Smooth a computed path |
| `/spin` | Rotate in place |
| `/undock_robot` | Undock the robot |
| `/wait` | Wait for a specified duration |

The most relevant actions for our warehouse search are
`/navigate_to_pose` and `/spin`. These allow the robot to
travel between search locations and rotate to inspect its
surroundings.

`/navigate_through_poses` is another possible option for
visiting multiple locations, although our current approach
uses individual navigation goals.

### MRTP Navigation Package

We inspected the MRTP navigation package, particularly:

- `navigation/include/navigation/navigation.hpp`
- `navigation/src/navigation.cpp`
- `navigation/src/testpackage.cpp`

The package provides a `Navigator` class that communicates
with Nav2 action servers and simplifies navigation commands.

The following methods will be useful for our project:

| Navigator Method | Purpose |
|---|---|
| `SetInitialPose()` | Set the robot's initial pose for localization |
| `WaitUntilNav2Active()` | Wait until AMCL and Nav2 are ready |
| `GoToPose()` | Send a navigation goal to a warehouse waypoint |
| `IsTaskComplete()` | Check whether the current action has finished |
| `GetResult()` | Retrieve the result of the navigation action |
| `Spin()` | Rotate the robot to inspect its surroundings |
| `CancelTask()` | Cancel the current action when necessary |
| `ClearAllCostmaps()` | Clear local and global costmaps during recovery |

We verified that `GoToPose()` sends goals to the
`/navigate_to_pose` action server using the `map` frame,
while `Spin()` communicates with the `/spin` action server.

Both methods return after the action goal has been accepted,
rather than waiting for the robot to complete the movement.
We therefore need `IsTaskComplete()` and `GetResult()` to
monitor the task and determine its outcome.

The `Spin()` method accepts an angle in radians and has a
default value of approximately 1.57 radians (90 degrees).

### Connection to Our Project

Our search system will initialize the robot's localization
and wait until Nav2 is active. It will then use `GoToPose()`
to navigate between predefined warehouse search locations.

At each location, the robot can use `Spin()` to rotate and
inspect its surroundings with the RGB-D camera. We will use
`IsTaskComplete()` and `GetResult()` to determine when
navigation or rotation has finished and whether it succeeded.

If necessary, `CancelTask()` can stop an active task, while
`ClearAllCostmaps()` may help recover from navigation problems.

These methods allow us to build our warehouse search behavior
without implementing the underlying Nav2 action clients
ourselves.

## 6. Find the two people already in the world (check the Gazebo entity tree). Write down their positions. You will use them to test your system.

We inspected the Gazebo Entity Tree and located the two
people already present in the warehouse simulation:
**Person 1 - Standing** and **Person 2 - Walking**.

By selecting each person in the Entity Tree and checking
the Pose component, we recorded their positions in the
Gazebo world coordinate system.

### Recorded Positions

| Entity | X (m) | Y (m) | Z (m) | Yaw (rad) |
|---|---:|---:|---:|---:|
| Person 1 - Standing | 1.00 | -1.00 | 0.00 | 1.57 |
| Person 2 - Walking | -12.00 | 15.00 | 0.00 | 0.00 |
| TurtleBot4 | 2.12 | -21.30 | 0.00 | 1.57 |

All three entities had roll and pitch values of 0.00 radians.

We also recorded the TurtleBot4's pose because it provides
a useful reference for understanding the robot's starting
location relative to the two people.

### Connection to Our Project

These positions will serve as reference coordinates for
testing our warehouse person-detection system.

Our system will use the RGB camera to detect people and
the depth image to estimate their 3D positions. We will
then use TF2 to transform those measurements into a
common coordinate frame.

By comparing the estimated positions with the positions
recorded in Gazebo, we can evaluate the accuracy of our
person-location estimates.

Since Person 2 is labeled as walking, we will also need
to verify whether its position changes during simulation
and record its current pose when testing.

We will also verify the relationship between Gazebo's
world frame and the Nav2 map frame before comparing
coordinates directly.
