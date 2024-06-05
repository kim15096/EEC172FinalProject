<template>
    <div class="laser-beam red"></div>
</template>

<script>
export default {
    name: 'LaserBeam'
}
</script>

<style scoped>
.laser-beam {
    position: absolute;
    left: 50%;
    bottom: 0;
    width: 3px;
    height: 200%;
    background: rgba(99, 195, 231, 0.6);
    box-shadow: 0px 0px 15px 0px rgba(99, 195, 231, 1);
    transform-origin: 0 100%;
    animation: laser 1s infinite;
}

.laser-beam.red {
    animation: laser 10s infinite;
    background: rgba(236, 19, 65, 0.6);
    box-shadow: 0px 0px 15px 0px rgba(236, 19, 65, 1);
}

@keyframes laser {
    0% {
        transform: rotateZ(-75deg);
    }

    50% {
        transform: rotateZ(75deg);
    }

    100% {
        transform: rotateZ(-75deg);
    }
}
</style>