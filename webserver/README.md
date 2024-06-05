# Setting up Webserver

Webserver using websockets and AWS IOT sdk and webrtc sdk.

## Getting Started

### Installing

```
npm install
```

### ENV variables

Create a .env file in the webserver directory with your AWS credentials.
```
AWS_ACCESS_KEY=""
AWS_SECRET_KEY=""
AWS_HOST_NAME=""
AWS_REGION=""
```

Make sure to also change the MQTT topic endpoints in aws_device.subscribe() to your AWS IOT specific endpoints.

### Executing program

```
node server.js
```
