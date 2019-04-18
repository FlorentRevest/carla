#!/usr/bin/env python

# Copyright (c) 2019 Florent Revest <revestflo@gmail.com>
# Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma de Barcelona (UAB).
#
# This work is licensed under the terms of the MIT license.
# For a copy, see <https://opensource.org/licenses/MIT>.

import glob
import os
import sys

try:
    sys.path.append(glob.glob('**/*%d.%d-%s.egg' % (sys.version_info.major, sys.version_info.minor, 'linux-x86_64'))[0])
except IndexError:
    pass

import carla

import random
import signal

if __name__ == '__main__':
    actor_list = []

    try:
        # Connect to CARLA
        client = carla.Client('localhost', 2000)
        client.set_timeout(2.0)
        world = client.get_world()
        blueprint_library = world.get_blueprint_library()

        # Create a vehicle
        vehicle_bp = blueprint_library.find('vehicle.nissan.patrol')
        vehicle_bp.set_attribute('role_name', 'hero')
        vehicle_bp.set_attribute('color', '255,255,255')
        vehicle_transform = world.get_map().get_spawn_points()[0]
        vehicle = world.spawn_actor(vehicle_bp, vehicle_transform)
        actor_list.append(vehicle)
        print('Created %s' % vehicle.type_id)

        # Create a LIDAR
        lidar_bp = blueprint_library.find('sensor.lidar.ray_cast')
	lidar_bp.set_attribute('channels', '40')
	lidar_bp.set_attribute('range', '20000.0') # Supposed to be in meters but wouldn't make sense
	lidar_bp.set_attribute('rotation_frequency', '10.0')
	lidar_bp.set_attribute('upper_fov', '7')
	lidar_bp.set_attribute('lower_fov', '-16')
        lidar_transform = carla.Transform(carla.Location(z=2.6))
        lidar = world.spawn_actor(lidar_bp, lidar_transform, attach_to=vehicle)
        actor_list.append(lidar)
        print('Created %s' % lidar.type_id)

        # Create five cameras (one with a narrow fov and four with a wide fov)
	for role_name in ['front_color', 'front', 'right', 'back', 'left']:
		camera_bp = blueprint_library.find('sensor.camera.rgb')
		camera_bp.set_attribute('role_name', role_name)
		camera_bp.set_attribute('image_size_x', '1280')
		camera_bp.set_attribute('image_size_y', '720')
		camera_bp.set_attribute('fov', '129')
		cam_z = 2.4
		cam_x = 0
		cam_y = 0
		cam_yaw = 0

		if role_name == 'front_color':
			camera_bp.set_attribute('fov', '52')
			cam_z = 2.3
			cam_y = 0.01
		elif role_name == 'front':
			cam_y = 0.01
		elif role_name == 'right':
			cam_x = 0.01
			cam_yaw = 90
		elif role_name == 'back':
			cam_y = -0.01
			cam_yaw = 180
		elif role_name == 'left':
			cam_x = -0.01
			cam_yaw = 270

		camera_transform = carla.Transform(carla.Location(x=cam_x, y=cam_y, z=cam_z),
                                                   carla.Rotation(yaw=cam_yaw))
		camera = world.spawn_actor(camera_bp, camera_transform, attach_to=vehicle)
		actor_list.append(camera)
		print('Created %s %s' % (camera.type_id, role_name))

        # Create a GNSS receiver
        gnss_bp = blueprint_library.find('sensor.other.gnss')
	gnss_bp.set_attribute('role_name', 'gnss')
        gnss = world.spawn_actor(gnss_bp, carla.Transform(), attach_to=vehicle)
        actor_list.append(gnss)
        print('Created %s' % gnss.type_id)

        signal.pause()

    finally:
        for actor in actor_list:
            actor.destroy()
        print('Destroyed actors')
