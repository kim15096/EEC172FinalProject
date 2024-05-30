<script>
import VideoPlayer from './components/VideoPlayer.vue'
import LazerBeam from './components/Lazer.vue';

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
  },
  methods: {
    setupWebSockets() {
      this.socket = new WebSocket('ws://localhost:3000');

      this.socket.onopen = () => {
        console.log("APP SOCKET CONNECTED")
      };

      this.socket.onmessage = (event) => {
        const message = JSON.parse(event.data);
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
