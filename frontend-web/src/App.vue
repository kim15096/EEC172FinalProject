<script setup>
import VideoPlayer from './components/VideoPlayer.vue'
</script>

<template>
  <div class="main d-flex flex-column">
    <h2 class="fw-bold">LazerCat</h2>
    <VideoPlayer />
  </div>
</template>

<style scoped>
.main {
  height: 100vh;
  width: 100vw;
  padding: 2rem;
}
</style>
