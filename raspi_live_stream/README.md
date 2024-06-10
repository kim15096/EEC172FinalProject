# Setting up Raspberry Pi for Live Streaming

## Getting Started

### Executing program
Create the executable kvsWebRTCMasterGStreamer from the sample provided by the aws iot sdk library. The sdk can be found here: https://github.com/awslabs/amazon-kinesis-video-streams-webrtc-sdk-c


We will be using cronjobs to execute our launcher python program on boot.

```
sudo crontab -e
```

Add the following line:
```
@reboot sleep 20; [path to your launcher script]
```
