<!-- src/components/Viewer.vue -->
<template>
  <div class="d-flex flex-column main-vid align-items-left">
    <div class="d-flex justify-content-between justify-content-center align-items-center align-text-center">
      <div>
        <Button @click="powerButton()" icon="pi pi-power-off" :severity='powerBtnSeverity' rounded
          aria-label="Filter" />
        <text class="ms-2 fw-bold" style="font-size: 16px;">{{ camPowerState }}</text>
      </div>
      <div class="mt-2">
        <text style="font-size: 14px; font-weight: 500;">{{ status }}</text>
      </div>
    </div>

    <video class="video-player mt-3" ref="remoteView" autoplay muted playsinline controls></video>

    <!-- Selection -->
    <Card v-if="!runningPreset" class="mt-3">
      <template #content>
        <text style="font-size: 18px;" class="fw-bold">Select Mode</text>
        <div class="d-flex justify-content-center mt-3 mb-3">
          <Button @click="presetCircleBtn()" size="small" severity="info" label="1. Circle"></Button>
          <Button @click="presetZigZagBtn()" size="small" severity="warn" class="ms-4 me-4" label="2. Zigzag"></Button>
          <Button @click="presetFastBtn()" size="small" severity="help" label="3. Fast"></Button>
        </div>
        <small style="color: gray;">*To control manually, use the remote controller</small>
      </template>
    </Card>
    <!-- Preset running -->
    <Card v-if="this.runningPreset" class="mt-3">
      <template #content>
        <text style="font-size: 18px;" class="fw-bold">Running: {{ runningPresetName }}</text>
        <ProgressBar class="mt-3 ms-4 me-4" mode="indeterminate" style="height: 6px"></ProgressBar>
        <div class="d-flex justify-content-center mt-3 mb-3">
          <Button @click="stopRunningPreset()" size="small" severity="danger" label="Stop"></Button>
        </div>
      </template>
    </Card>
    <!-- Manual mode -->
    <Card v-if="this.runningManual" class="mt-3">
      <template #content>
        <text style="font-size: 18px;" class="fw-bold">Manual Lazer Mode</text>
        <ProgressBar class="mt-3 ms-4 me-4" mode="indeterminate" style="height: 6px"></ProgressBar>
        <div class="d-flex justify-content-center mt-3 mb-3">
          <Button @click="stopRunningPreset()" size="small" severity="danger" label="Stop"></Button>
        </div>
      </template>
    </Card>
  </div>
</template>

<script>
import axios from 'axios';
import Card from 'primevue/card';
import Button from 'primevue/button';
import ProgressBar from 'primevue/progressbar';

export default {
  data() {
    return {
      signalingClient: null,
      peerConnection: null,
      camPowerState: 'CAM OFF',
      powerBtnSeverity: 'danger',
      status: 'Connecting...',
      messages: null,
      socket: null,
      runningPreset: false,
      runningManual: false,
      runningPresetName: '',
    };
  },
  components: {
    Button,
    Card,
    ProgressBar,
  },
  methods: {
    getCurrentState() {
      const url = import.meta.env.VITE_SERVER_URL + "/getShadowState"
      axios.get(url).then((response) => {
        const home_pi_cam_state = response.data.state.desired.home_pi_cam_state
        const home_cc_input = response.data.state.desired.home_cc_input
        const home_cc_state = response.data.state.desired.home_cc_state

        if (home_pi_cam_state == "IDLE") {
          this.camPowerState = 'CAM OFF'
          this.powerBtnSeverity = 'danger'
        }
        else {
          this.camPowerState = 'CAM ON'
          this.powerBtnSeverity = 'success'
        }

        if (home_cc_state == "MANUAL") {
          this.runningManual = true;
        }

        if (home_cc_input == "CIRCLE" || home_cc_input == "ZIGZAG" || home_cc_input == "FAST") {
          this.runningPreset = true
          this.runningPresetName = home_cc_input[0] + home_cc_input.toLowerCase().slice(1)
        }
        else {
          this.runningPreset = false
          this.runningPresetName = ''
        }
      })
    },
    presetCircleBtn() {
      const req_body = {
        "state": {
          "desired": {
            "home_cc_state": "AUTO",
            "home_cc_input": "CIRCLE",
          }
        }
      }
      const url = import.meta.env.VITE_SERVER_URL + "/updateShadowState"
      axios.post(url, req_body).then((response) => {
        console.log(response.data.message)
        this.runningPreset = true
        this.runningPresetName = 'Circle'
      })
    },
    presetZigZagBtn() {
      const req_body = {
        "state": {
          "desired": {
            "home_cc_state": "AUTO",
            "home_cc_input": "ZIGZAG",
          }
        }
      }

      const url = import.meta.env.VITE_SERVER_URL + "/updateShadowState"
      axios.post(url, req_body).then((response) => {
        console.log(response.data.message)
        this.runningPreset = true
        this.runningPresetName = 'Zigzag'
      })
    },
    presetFastBtn() {
      const req_body = {
        "state": {
          "desired": {
            "home_cc_state": "AUTO",
            "home_cc_input": "FAST",
          }
        }
      }

      const url = import.meta.env.VITE_SERVER_URL + "/updateShadowState"
      axios.post(url, req_body).then((response) => {
        console.log(response.data.message)
        this.runningPreset = true
        this.runningPresetName = 'Fast'
      })
    },
    stopRunningPreset() {
      const req_body = {
        "state": {
          "desired": {
            "home_cc_state": "IDLE",
            "home_cc_input": "NONE",
          }
        }
      }

      const url = import.meta.env.VITE_SERVER_URL + "/updateShadowState"
      axios.post(url, req_body).then((response) => {
        console.log(response.data.message)
        this.runningPreset = false
        this.runningPresetName = ''
      })
    },
    powerButton() {

      if (this.camPowerState == 'CAM ON') {

        const req_body = {
          "state": {
            "desired": {
              "home_pi_cam_state": "IDLE",
            }
          }
        }

        const url = import.meta.env.VITE_SERVER_URL + "/updateShadowState"
        axios.post(url, req_body).then((response) => {
          console.log(response.data.message)
          this.camPowerState = 'CAM OFF';
          this.powerBtnSeverity = 'danger'
        })

      }
      else {

        const req_body = {
          "state": {
            "desired": {
              "home_pi_cam_state": "ON",
            }
          }
        }

        const url = import.meta.env.VITE_SERVER_URL + "/updateShadowState"
        axios.post(url, req_body).then((response) => {
          this.camPowerState = 'CAM ON';
          this.powerBtnSeverity = 'success'
          console.log(response.data.message)
        })
      }
    },
    setupWebSocket() {
      this.socket = new WebSocket('wss://eec172finalproject-production.up.railway.app');

      this.socket.onopen = () => {
        console.log("VIDEOPLAYER SOCKET CONNECTED")
        this.status = 'Connected!';
      };

      this.socket.onmessage = (event) => {
        const message = JSON.parse(event.data);
        console.log("SOCKET MESSAGE", message)

        const home_pi_cam_state = message.data.state.desired.home_pi_cam_state;
        const home_cc_state = message.data.state.desired.home_cc_state;
        const home_cc_input = message.data.state.desired.home_cc_input;

        if (home_pi_cam_state == 'ON') {
          this.camPowerState = 'CAM ON';
          this.powerBtnSeverity = 'success'
        } else if (home_pi_cam_state == 'IDLE') {
          this.camPowerState = 'CAM OFF';
          this.powerBtnSeverity = 'danger'
        }

        if (home_cc_state == "MANUAL") {
          this.runningManual = true;
        }

        if (home_cc_input == "CIRCLE" || home_cc_input == "ZIGZAG" || home_cc_input == "FAST") {
          this.runningPreset = true
          this.runningPresetName = home_cc_input[0] + home_cc_input.toLowerCase().slice(1)
        }
        else {
          this.runningPreset = false;
          this.runningPresetName = ''
        }

      };

      this.socket.onclose = () => {
        this.status = 'Disconnected!';
      };

      this.socket.onerror = (error) => {
        console.error('WebSocket Error:', error);
      };
    },
    async setupViewer() {
      try {
        // DEV MODE
        // const response = await axios.get('http://localhost:3000/viewer', {
        //   params: {
        //     ChannelARN: import.meta.env.VITE_AWS_SIGNAL_ARN,
        //     ClientId: 'device-test',
        //   },
        // });

        // PROD MODE
        const viewer_url = import.meta.env.VITE_SERVER_URL + '/viewer'

        const response = await axios.get(viewer_url, {
          params: {
            ChannelARN: import.meta.env.VITE_AWS_SIGNAL_ARN,
            ClientId: 'device-test',
          },
        });

        const {
          channelARN,
          channelEndpoint,
          clientId,
          region,
          iceServers,
          accessKeyId,
          secretAccessKey,
          systemClockOffset,
        } = response.data;

        this.signalingClient = new window.KVSWebRTC.SignalingClient({
          channelARN,
          channelEndpoint,
          clientId,
          role: window.KVSWebRTC.Role.VIEWER,
          region,
          credentials: { accessKeyId, secretAccessKey },
          systemClockOffset,
        });

        this.peerConnection = new RTCPeerConnection({ iceServers });

        this.peerConnection.addEventListener('icecandidate', ({ candidate }) => {
          if (candidate) {
            this.signalingClient.sendIceCandidate(candidate);
          }
        });

        this.peerConnection.addEventListener('track', (event) => {
          this.$refs.remoteView.srcObject = event.streams[0];
        });

        this.signalingClient.on('sdpAnswer', async (answer) => {
          await this.peerConnection.setRemoteDescription(answer);
        });

        this.signalingClient.on('iceCandidate', (candidate) => {
          this.peerConnection.addIceCandidate(candidate);
        });

        this.signalingClient.on('open', async () => {
          const offer = await this.peerConnection.createOffer({
            offerToReceiveAudio: true,
            offerToReceiveVideo: true,
          });
          await this.peerConnection.setLocalDescription(offer);
          this.signalingClient.sendSdpOffer(this.peerConnection.localDescription);
        });

        this.signalingClient.open();
      } catch (error) {
        console.error('Error setting up viewer:', error);
      }
    },
  },
  mounted() {
    this.getCurrentState();
    this.setupViewer();
    this.setupWebSocket();
  },
  beforeDestroy() {
    if (this.socket) {
      this.socket.close();
    }
  },
};
</script>

<style scoped>
.main-vid {
  background-color: transparent;
  width: fit-content;
}

.video-player {
  border-radius: 1rem;
  width: 100%;
  box-shadow: 0 2px 50px 0 rgba(0, 0, 0, 0.15);
  transform: scale(-1, 1);
  -webkit-transform: rotateY(180deg);
  /* Safari and Chrome */
  -moz-transform: rotateY(180deg);
  /* Firefox */
}

video::-webkit-media-controls-panel {
  transform: scale(-1, 1);
  -webkit-transform: rotateY(180deg);
  -moz-transform: rotateY(180deg);
}
</style>
