<script>
import VideoPlayer from './components/VideoPlayer.vue'
import LazerBeam from './components/Lazer.vue';
import axios from 'axios';

export default {
  data() {
    return {
      socket: null,
      isLazerOn: false,
    }
  },
  components: {
    VideoPlayer,
    LazerBeam,
  },
  mounted() {
    this.setupWebSockets();
    this.getCurrentState();
  },
  methods: {
    getCurrentState() {
      const url = import.meta.env.VITE_SERVER_URL + "/getShadowState"
      axios.get(url).then((response) => {
        const home_pi_cam_state = response.data.state.desired.home_pi_cam_state
        const home_cc_input = response.data.state.desired.home_cc_input
        const home_cc_state = response.data.state.desired.home_cc_state

        if (home_cc_state == "MANUAL") {
          this.isLazerOn = true;
        }
        else {
          this.isLazerOn = false
        }

        if (home_cc_input == "CIRCLE" || home_cc_input == "ZIGZAG" || home_cc_input == "FAST") {
          this.isLazerOn = true
        }
        else {
          this.isLazerOn = false
        }
      })
    },
    setupWebSockets() {
      this.socket = new WebSocket('ws://0.0.0.0:3000');

      this.socket.onopen = () => {
        console.log("APP SOCKET CONNECTED")
      };

      this.socket.onmessage = (event) => {
        const message = JSON.parse(event.data);
        console.log(message)
        const home_lazer_state = message.data.state.desired.home_cc_state
        if (home_lazer_state == "MANUAL" || home_lazer_state == "AUTO") {
          this.isLazerOn = true
        }
        else {
          this.isLazerOn = false
        }
      };
    }
  }
}
</script>


<template>
  <div class="main d-flex flex-column align-items-center">
    <img draggable=false src="./assets/LazerCat_Icon.png" style="width: 80px">
    <LazerBeam v-if="isLazerOn"></LazerBeam>
    <VideoPlayer class="video-player-main" />
  </div>
</template>

<style scoped>
.main {
  height: 100vh;
  width: 100vw;
  padding: 2rem;
  position: relative;
  overflow-y: scroll;
  overflow-x: hidden;
}

.video-player-main {
  width: 85vw;
  max-width: 800px;
  transform: translateY(-50px);
}
</style>
