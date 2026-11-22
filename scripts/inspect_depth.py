import math
import struct

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image


class DepthInspector(Node):
    def __init__(self):
        super().__init__('depth_inspector')

        self.subscription = self.create_subscription(
            Image,
            '/rgbd_camera/depth_image',
            self.inspect,
            10
        )

    def inspect(self, msg):
        if msg.encoding != '32FC1':
            print(f'Unexpected encoding: {msg.encoding}')
            rclpy.shutdown()
            return

        values = []
        endian = '>' if msg.is_bigendian else '<'

        for row in range(msg.height):
            start = row * msg.step
            row_bytes = msg.data[start:start + msg.width * 4]

            values.extend(
                struct.unpack(f'{endian}{msg.width}f', row_bytes)
            )

        finite = [v for v in values if math.isfinite(v)]

        print(f'Resolution: {msg.width} x {msg.height}')
        print(f'Encoding: {msg.encoding}')
        print(f'Total pixels: {len(values)}')
        print(f'Finite pixels: {len(finite)}')
        print(f'NaN pixels: {sum(math.isnan(v) for v in values)}')
        print(f'+Infinity pixels: {sum(v == math.inf for v in values)}')
        print(f'-Infinity pixels: {sum(v == -math.inf for v in values)}')
        print(f'Zero pixels: {sum(v == 0.0 for v in values)}')

        if finite:
            print(f'Minimum finite depth: {min(finite):.4f}')
            print(f'Maximum finite depth: {max(finite):.4f}')

        center = (msg.height // 2) * msg.width + (msg.width // 2)
        print(f'Center pixel depth: {values[center]}')

        rclpy.shutdown()


rclpy.init()
node = DepthInspector()

try:
    rclpy.spin(node)
finally:
    node.destroy_node()

    if rclpy.ok():
        rclpy.shutdown()
