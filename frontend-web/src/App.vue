<script setup>
import VideoPlayer from './components/VideoPlayer.vue'
import LazerBeam from './components/Lazer.vue';
</script>

<template>
  <div class="main d-flex flex-column align-items-center">
    <img draggable=false src="./assets/LazerCat_Icon.png" style="width: 80px">
    <LazerBeam></LazerBeam>
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
