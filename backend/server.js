const express = require('express');
const bodyParser = require('body-parser');
const awsIot = require('aws-iot-device-sdk');
const app = express();
const AWS = require('aws-sdk');
const { SignalingClient, Role } = require('amazon-kinesis-video-streams-webrtc');
const path = require('path');
const cors = require('cors');
const https = require('https');
const WebSocket = require('ws');
require('dotenv').config();

const port = 3000;

app.use(cors());
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, 'public')));

// Create web sockets
const server = https.createServer(app);
const wss = new WebSocket.Server({ server });

// Broadcast function to send data to socket clients
function broadcast(data) {
    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify(data));
        }
    });
}

// SETUP AWS Config for credentials
AWS.config.update({
    region: process.env.AWS_REGION,
    accessKeyId: process.env.AWS_ACCESS_KEY,
    secretAccessKey: process.env.AWS_SECRET_KEY,
});

// SETUP IotData for GET requests
const iotData = new AWS.IotData({ endpoint: process.env.AWS_HOST_NAME });

// SETUP AWS KINESIS FOR VIDEO STREAM
const kinesisVideoClient = new AWS.KinesisVideo({
    region: process.env.AWS_REGION,
    correctClockSkew: true,
    reconnectPeriod: 10000
});

// SETUP CONNECTIONS
const aws_device = awsIot.device({
    clientId: 'device-test',
    host: process.env.AWS_HOST_NAME,
    protocol: 'wss',
    secretKey: process.env.AWS_SECRET_KEY,
    accessKeyId: process.env.AWS_ACCESS_KEY,
    region: process.env.AWS_REGION,
    reconnectPeriod: 10000,
});

let clientTokenGet;
let stateObjectStore = null;
let clientTokenUpdate;

aws_device.on('connect', function () {
    console.log("Device connected to AWS IoT");

    aws_device.subscribe('$aws/things/andrew_cc3200/shadow/update/accepted');
    aws_device.subscribe('$aws/things/andrew_cc3200/shadow/update/rejected');
    aws_device.subscribe('$aws/things/andrew_cc3200/shadow/update/delta');
});

aws_device.on('status', function (thingName, stat, clientToken, stateObject) {
    console.log(`Received ${stat} on ${thingName}: ${JSON.stringify(stateObject, null, 2)}`);

    if (clientToken === clientTokenGet) {
        stateObjectStore = stateObject;
    } else if (clientToken === clientTokenUpdate) {
        broadcast({ type: 'update', data: stateObject });
    }
});

aws_device.on('message', function (topic, payload) {
    console.log(`Received message on topic ${topic}: ${payload.toString()}`);

    if (topic === '$aws/things/andrew_cc3200/shadow/update/accepted') {
        const update = JSON.parse(payload.toString());
        console.log(update);
        broadcast({ type: 'update', data: update });
    }
});

aws_device.on('error', function (error) {
    console.log('Error:', error);
});

aws_device.on('timeout', function (thingName, clientToken) {
    console.log(`Timeout on ${thingName} with token: ${clientToken}`);
});

// GET SHADOW STATE
app.get('/getShadowState', (req, res) => {

    const params = {
        thingName: "andrew_cc3200"
    }

    iotData.getThingShadow(params, (err, data) => {
        if (err) {
            console.log(err, err.stack); // an error occurred
            res.status(500).send({ error: 'Error retrieving shadow state' });
        } else {
            const payload = JSON.parse(data.payload);
            res.send(payload);
        }
    });
});

// UPDATE SHADOW STATE
app.post('/updateShadowState', (req, res) => {
    const state = req.body;
    aws_device.publish('$aws/things/andrew_cc3200/shadow/update', JSON.stringify(state), (err) => {
        if (err) {
            console.error('Error updating shadow state:', err);
            res.status(500).json({ message: 'Error updating shadow state', error: err });
        } else {
            res.json({ message: 'Update Successful!' });
        }
    });
});

// DELETE SHADOW STATE
app.post('/deleteShadowState', (req, res) => {
    const state = req.body;

    aws_device.publish('$aws/things/andrew_cc3200/shadow/delete', JSON.stringify(state), (err) => {
        if (err) {
            console.error('Error deleting shadow state:', err);
            res.status(500).json({ message: 'Error deleting shadow state', error: err });
        } else {
            res.json({ message: 'Delete Successful!' });
        }
    });
});

// Get streaming info
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
server.listen(port, '0.0.0.0', () => {
    console.log(`Server is running on http://0.0.0.0:${port}`);
});
