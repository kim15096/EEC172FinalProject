// SERVER SETUP
const express = require('express');
const bodyParser = require('body-parser');
const awsIot = require('aws-iot-device-sdk');
const app = express();
const AWS = require('aws-sdk');
const { SignalingClient, Role } = require('amazon-kinesis-video-streams-webrtc');
const path = require('path');
var cors = require('cors')
require('dotenv').config();

const port = 3000;

//comment
app.use(cors())
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, 'public')));

// SETUP AWS KINESIS FOR VIDEO STREAM
AWS.config.update({
    region: process.env.AWS_REGION,
    accessKeyId: process.env.AWS_ACCESS_KEY,
    secretAccessKey: process.env.AWS_SECRET_KEY,
});

const kinesisVideoClient = new AWS.KinesisVideo({
    region: process.env.AWS_REGION,
    correctClockSkew: true,
});

// SETUP DEVICE SHADOW CONNECTION
const thingShadows = awsIot.device({
    clientId: 'device-test',
    host: process.env.AWS_HOST_NAME,
    protocol: 'wss',
    secretKey: process.env.AWS_SECRET_KEY,
    accessKeyId: process.env.AWS_ACCESS_KEY,
    region: process.env.AWS_REGION,
});

let clientTokenGet;
let stateObjectStore = null;
let clientTokenUpdate;

thingShadows.on('connect', function () {
    console.log("Connected to AWS IoT");

    thingShadows.subscribe('$aws/things/andrew_cc3200/shadow/update/accepted');
    thingShadows.subscribe('$aws/things/andrew_cc3200/shadow/update/rejected');
    thingShadows.subscribe('$aws/things/andrew_cc3200/shadow/update/delta');
});

thingShadows.on('status', function (thingName, stat, clientToken, stateObject) {
    console.log(`Received ${stat} on ${thingName}: ${JSON.stringify(stateObject, null, 2)}`);

    if (clientToken === clientTokenGet) {
        console.log(`Received ${stat} on ${thingName}: ${JSON.stringify(stateObject, null, 2)}`);
        stateObjectStore = stateObject;
    } else if (clientToken === clientTokenUpdate) {
        console.log(`Update ${stat} on ${thingName}: ${JSON.stringify(stateObject, null, 2)}`);
    }
});

thingShadows.on('message', function (topic, payload) {
    console.log(`Received message on topic ${topic}: ${payload.toString()}`);

    if (topic === '$aws/things/andrew_cc3200/shadow/update/accepted') {
        const delta = JSON.parse(payload.toString());
        console.log(delta)
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

app.get('/viewer', async (req, res) => {
    try {
        const { ChannelARN, ClientId } = req.query;

        // Get signaling channel endpoints
        const getSignalingChannelEndpointResponse = await kinesisVideoClient
            .getSignalingChannelEndpoint({
                ChannelARN,
                SingleMasterChannelEndpointConfiguration: {
                    Protocols: ['WSS', 'HTTPS'],
                    Role: Role.VIEWER,
                },
            })
            .promise();

        const endpointsByProtocol = getSignalingChannelEndpointResponse.ResourceEndpointList.reduce((endpoints, endpoint) => {
            endpoints[endpoint.Protocol] = endpoint.ResourceEndpoint;
            return endpoints;
        }, {});

        const kinesisVideoSignalingChannelsClient = new AWS.KinesisVideoSignalingChannels({
            region: process.env.AWS_REGION,
            endpoint: endpointsByProtocol.HTTPS,
            correctClockSkew: true,
        });

        const getIceServerConfigResponse = await kinesisVideoSignalingChannelsClient
            .getIceServerConfig({ ChannelARN })
            .promise();

        const iceServers = [
            { urls: `stun:stun.kinesisvideo.${process.env.AWS_REGION}.amazonaws.com:443` },
            ...getIceServerConfigResponse.IceServerList.map(iceServer => ({
                urls: iceServer.Uris,
                username: iceServer.Username,
                credential: iceServer.Password,
            })),
        ];

        res.json({
            channelARN: ChannelARN,
            channelEndpoint: endpointsByProtocol.WSS,
            clientId: ClientId,
            region: process.env.AWS_REGION,
            iceServers,
            accessKeyId: process.env.AWS_ACCESS_KEY,
            secretAccessKey: process.env.AWS_SECRET_KEY,
            systemClockOffset: kinesisVideoClient.config.systemClockOffset,
        });

    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// START SERVER ON PORT 3000
app.listen(port, '0.0.0.0', () => {
    console.log(`Server is running on http://localhost:${port}`);
});
