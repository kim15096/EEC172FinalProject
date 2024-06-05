# Setting up Raspberry Pi for Live Streaming

## Getting Started

### Executing program

We will be using cronjobs to execute our launcher python program on boot.

```
sudo crontab -e
```

Add the following line:
```
@reboot sleep 20; [path to your launcher script]
```

## Acknowledgments
