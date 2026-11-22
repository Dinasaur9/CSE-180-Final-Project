
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

