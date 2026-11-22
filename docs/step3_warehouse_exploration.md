
# Step 3 — Warehouse Exploration

**Course:** CSE 180 — Introduction to Robotics  
**Project:** Search the Warehouse  
**Simulation:** `tb4_warehouse.launch.py`

## 1. RGB Image, Depth Image, and Camera Calibration

We explored the warehouse simulation using `ros2 topic list -t` and `ros2 topic echo` to identify the topics associated with the TurtleBot4's RGB-D camera.

### Camera Topics and Message Types

| Information | ROS2 Topic | Message Type | Resolution | Encoding |
|---|---|---|---|---|
| RGB color image | `/rgbd_camera/image` | `sensor_msgs/msg/Image` | 320 × 240 | `rgb8` |
| Depth image | `/rgbd_camera/depth_image` | `sensor_msgs/msg/Image` | 320 × 240 | `32FC1` |
| Camera calibration | `/rgbd_camera/camera_info` | `sensor_msgs/msg/CameraInfo` | 320 × 240 (previous observation) | N/A |

Both image messages use the coordinate frame `oakd_rgb_camera_optical_frame`. The camera calibration message previously reported the same frame; this still needs to be reconfirmed in the warehouse session.

### Understanding Our Findings

The RGB image uses `rgb8` encoding, meaning each pixel contains three 8-bit color channels (red, green, and blue). This image will be used by our neural network to detect people.

The depth image uses `32FC1` encoding, meaning each pixel contains one 32-bit floating-point depth measurement. Valid depth values conventionally represent meters. We will verify how the simulator handles invalid measurements separately.

Both images have a resolution of 320 × 240 pixels. The matching dimensions make it easier to associate RGB detections with depth measurements, although spatial alignment and timestamps must also be considered.

The camera calibration topic provides the intrinsic parameters needed to convert pixel coordinates into 3D positions.

### Commands Used

```bash
ros2 topic list -t | grep 'rgbd_camera'

ros2 topic echo /rgbd_camera/image --once --field header
ros2 topic echo /rgbd_camera/image --once --field encoding
ros2 topic echo /rgbd_camera/image --once --field height
ros2 topic echo /rgbd_camera/image --once --field width

ros2 topic echo /rgbd_camera/depth_image --once --field header
ros2 topic echo /rgbd_camera/depth_image --once --field encoding
ros2 topic echo /rgbd_camera/depth_image --once --field height
ros2 topic echo /rgbd_camera/depth_image --once --field width

ros2 topic echo /rgbd_camera/camera_info --once
```

**Status:** RGB and depth image properties verified in the warehouse. Camera calibration values still need to be reconfirmed.

---

## 2. Camera Intrinsics and Pinhole Camera Model

We inspected the camera calibration information from `/rgbd_camera/camera_info` in our earlier simulation session.

The camera intrinsic matrix was:

\[
K =
\begin{bmatrix}
221.765 & 0 & 160 \\
0 & 221.765 & 120 \\
0 & 0 & 1
\end{bmatrix}
\]

### Intrinsic Parameters

| Parameter | Array Entry | Value | Description |
|---|---|---|---|
| \(f_x\) | `k[0]` | 221.765 | Horizontal focal length in pixels |
| \(f_y\) | `k[4]` | 221.765 | Vertical focal length in pixels |
| \(c_x\) | `k[2]` | 160 | Horizontal principal point |
| \(c_y\) | `k[5]` | 120 | Vertical principal point |

These values should be reconfirmed using the warehouse launch file.

### Pinhole Camera Model

The pinhole camera model relates a 3D point \((X,Y,Z)\) in the camera optical frame to its image coordinates \((u,v)\):

\[
u=f_x\frac{X}{Z}+c_x
\]

\[
v=f_y\frac{Y}{Z}+c_y
\]

By rearranging these equations and using the optical-axis depth \(d=Z\), we can recover the 3D point:

\[
\boxed{X=\frac{(u-c_x)d}{f_x}}
\]

\[
\boxed{Y=\frac{(v-c_y)d}{f_y}}
\]

\[
\boxed{Z=d}
\]

### Example

Suppose a person is detected at pixel \((200,140)\) and the depth image reports 3.0 meters.

Using the previously measured camera intrinsics:

\[
X=\frac{(200-160)(3)}{221.765}\approx0.541
\]

\[
Y=\frac{(140-120)(3)}{221.765}\approx0.271
\]

\[
Z=3.0
\]

The resulting 3D point is:

\[
\boxed{P_{\text{camera}}=(0.541,\ 0.271,\ 3.0)\text{ m}}
\]

These coordinates describe the point relative to the camera's optical frame. We will later use TF2 to transform the point into the warehouse's `map` frame.

### Why This Matters

Our person-detection system will use the RGB image to locate people in pixel coordinates. We will then use the depth image and camera intrinsic parameters to estimate their 3D positions. Finally, TF2 will transform those positions into map coordinates so the robot can report where people were found.

**Status:** Pinhole equations understood. Camera intrinsic values pending reconfirmation in the warehouse.
