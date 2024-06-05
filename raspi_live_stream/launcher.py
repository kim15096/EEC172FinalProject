import os
import subprocess
import websockets
import asyncio
import json
from dotenv import load_dotenv

load_dotenv()

process = None

command = ['./kvsWebrtcClientMasterGstSample', 'live_streaming', 'video-only', 'devicesrc']

async def connect_websocket():
	uri = "ws://eec172finalproject-production.up.railway.app"
	async with websockets.connect('wss://eec172finalproject-production.up.railway.app') as websocket:
		print("WEBSOCKET CONNECTED")
		
		while True:
			try: 
				message = await websocket.recv()
				message_data = json.loads(message)

				pi_camera_state = message_data['data']['state']['desired']['home_pi_cam_state']

				if pi_camera_state:
					print(f"CAMERA STATE is {pi_camera_state}")
					if pi_camera_state == "ON":
						start_executable()
					else:
						kill_executable()
				
			except websockets.ConnectionClosed:
				print("Connection closed!")

			except Exception as e:
				print(f"Error: {e}")

def start_executable():
	global process
	process = subprocess.Popen(command)

def kill_executable():
	process.terminate()
	exit_code = process.wait()
	print(f"Process killed with exit code {exit_code}")

def main():
	print("PYTHON SCRIPT RUNNING ON BOOT UP!")
	asyncio.get_event_loop().run_until_complete(connect_websocket())

if __name__ == "__main__":
	main()
			
