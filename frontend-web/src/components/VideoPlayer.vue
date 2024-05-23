<script>
import { ref } from 'vue'
import axios from 'axios';

export default {
  data() {

  },
  mounted() {
    this.fetchStreamUrl();
  },
  methods: {
    async fetchStreamUrl() {
      try {
        const response = await axios.get('http://localhost:3000/get-stream-url');
        const data = response.data;
        const videoPlayer = document.getElementById('videoPlayer');
        videoPlayer.src = data.url;
      } catch (error) {
        console.error('Error fetching stream URL:', error);
      }
    }
  },

}
</script>

<template>
  <h1>Raspberry Pi Camera Stream</h1>
  <video id="videoPlayer" controls autoplay></video>
</template>

<style scoped>
.read-the-docs {
  color: #888;
}
</style>
