// SERVER SETUP
const express = require('express');
const bodyParser = require('body-parser');
const awsIot = require('aws-iot-device-sdk');
const app = express();
const AWS = require('aws-sdk');
const path = require('path');
var cors = require('cors')
require('dotenv').config();

const port = 3000;

app.use(cors())
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, 'public')));

// SETUP AWS KINESIS FOR VIDEO STREAM
AWS.config.update({
    region: 'us-east-1',
    accessKeyId: process.env.AWS_ACCESS_KEY,
    secretAccessKey: process.env.AWS_SECRET_KEY,
});

const kinesisVideo = new AWS.KinesisVideo();
const kinesisVideoArchivedMedia = new AWS.KinesisVideoArchivedMedia();

// SETUP DEVICE SHADOW CONNECTION
const thingShadows = awsIot.thingShadow({
    // keyPath: 'private.pem.key',
    // certPath: 'certificate.pem.crt',
    // caPath: 'rootCA.pem',
    clientId: 'device-test',
    host: 'a2hn94z4q1ycvj-ats.iot.us-east-1.amazonaws.com',
    protocol: 'wss', // Ensure secure protocol
    reconnectPeriod: 1000, // Set a reconnect period if connection drops
    keepalive: 10, // Set keepalive interval
    secretKey: process.env.AWS_SECRET_KEY,
    accessKeyId: process.env.AWS_ACCESS_KEY,
    region: 'us-east-1',
});

let clientTokenGet;
let stateObjectStore = null;
let clientTokenUpdate;

thingShadows.on('connect', function () {
    console.log("Connected to AWS IoT");

    // Register the thing name
    thingShadows.register('andrew_cc3200', {}, function () {
        console.log("Registered andrew_cc3200");
    });
});

thingShadows.on('status', function (thingName, stat, clientToken, stateObject) {
    if (clientToken === clientTokenGet) {
        console.log(`Received ${stat} on ${thingName}: ${JSON.stringify(stateObject, null, 2)}`);
        stateObjectStore = stateObject;
    } else if (clientToken === clientTokenUpdate) {
        console.log(`Update ${stat} on ${thingName}: ${JSON.stringify(stateObject, null, 2)}`);
    }
});

thingShadows.on('error', function (error) {
    console.log('Error:', error);
});

thingShadows.on('timeout', function (thingName, clientToken) {
    console.log(`Timeout on ${thingName} with token: ${clientToken}`);
});

// GET SHADOW STATE
app.get('/getShadowState', (req, res) => {
    clientTokenGet = thingShadows.get('andrew_cc3200');
    console.log(`Client Token: ${clientTokenGet}`);

    if (clientTokenGet === null) {
        console.log('Shadow get request failed');
        res.status(500).send('Shadow get request failed');
    } else {
        setTimeout(() => {
            if (stateObjectStore) {
                res.json(stateObjectStore);
            } else {
                res.status(500).send('Failed to retrieve the state object');
            }
        }, 2000);
    }
});

// UPDATE SHADOW STATE
app.post('/updateShadowState', (req, res) => {
    const newState = req.body;

    if (!newState.state) {
        res.status(400).send('Invalid request: "state" field is required in the body');
        return;
    }

    clientTokenUpdate = thingShadows.update('andrew_cc3200', newState);

    if (clientTokenUpdate === null) {
        console.log('Shadow update request failed');
        res.status(500).send('Shadow update request failed');
    } else {
        res.send(`Shadow update request sent with token: ${clientTokenUpdate}`);
    }
});

// GET STREAM URL
app.get('/get-stream-url', async (req, res) => {
    try {
        const streamName = 'video_stream';
        const describeStreamResponse = await kinesisVideo.describeStream({ StreamName: streamName }).promise();
        const streamARN = describeStreamResponse.StreamInfo.StreamARN;

        console.log(streamARN)

        const getDataEndpointResponse = await kinesisVideo.getDataEndpoint({
            StreamARN: streamARN,
            APIName: 'GET_HLS_STREAMING_SESSION_URL'
        }).promise();

        const endpoint = getDataEndpointResponse.DataEndpoint;

        kinesisVideoArchivedMedia.endpoint = new AWS.Endpoint(endpoint);

        const hlsStreamResponse = await kinesisVideoArchivedMedia.getHLSStreamingSessionURL({
            StreamName: streamName,
            PlaybackMode: 'LIVE'
        }).promise();

        res.json({ url: hlsStreamResponse.HLSStreamingSessionURL });
    } catch (err) {
        console.error(err);
        res.status(500).send('Error getting stream URL');
    }
});

// START SERVER ON PORT 3000
app.listen(port, () => {
    console.log(`Server is running on http://localhost:${port}`);
});
