<!-- src/components/Viewer.vue -->
<template>
  <div class="d-flex flex-column align-items-center p-3">
    <video class="video-player" ref="remoteView" autoplay playsinline controls></video>
  </div>
</template>

<script>
import axios from 'axios';

export default {
  data() {
    return {
      signalingClient: null,
      peerConnection: null,
    };
  },
  methods: {
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
        const response = await axios.get(import.meta.env.VITE_SERVER_URL + '/viewer', {
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

        console.log(response.data)

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
    this.setupViewer();
  },
};
</script>

<style scoped>
.video-player {
  background-color: black;
  max-width: 1000px;
  border-radius: 1rem;
  width: 100%;
  box-shadow: 0 2px 50px 0 rgba(0, 0, 0, 0.15);
}
</style>
