import Schema from "async-validator";

export const DEVICE_SETTINGS_VALIDATOR = new Schema({
    dev_eui: {
        required: true, message: "Please provide an id"
    },
    token: {
        required: true, message: "Please provide a token"
    },
    server: {
        required: true, message: "Please provide a server domain"
    },
    path: {
        required: true, message: "Please provide a server path"
    }
});

export const LIGHT_MQTT_SETTINGS_VALIDATOR = new Schema({
    unique_id: {
        required: true, message: "Please provide an id"
    },
    name: {
        required: true, message: "Please provide a name"
    },
    mqtt_path: {
        required: true, message: "Please provide an MQTT path"
    }
});
