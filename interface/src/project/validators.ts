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
    },
    enabled: {
        required: false, message: "Please provide to enabled Wifi Transmission"
    }
});

export const DEVICE_LORAWAN_SETTINGS_VALIDATOR = new Schema({
    dev_eui: {
        required: false, message: "Please provide an id"
    },
    app_eui: {
        required: false, message: "Please provide a App EUI"
    },
    app_key: {
        required: false, message: "Please provide a APP Key"
    },
    net_key: {
        required: false, message: "Please provide a Network Key"
    },
    dev_address: {
        required: false, message: "Please provide a Device Address"
    },
    apps_key: {
        required: false, message: "Please provide a Application Session Key"
    },
    nets_key: {
        required: false, message: "Please provide a Network Session Key"
    },
    use_otaa: {
        required: false, message: "Please Define if use OTAA or ABP"
    },
    class_mode: {
        required: false, message: "Please provide a class mode"
    },
    band: {
        type: "number", required: false
    },
    sub_band: {
        type: "number", required: false
    },
    data_rate: {
        type: "number", required: false
    },
    rx2_dr: {
        type: "number", required: false
    },
    rx2_freq_hz: {
        type: "number", required: false
    },
    adr: {
        type: "boolean", required: false
    },
    confirm_mode: {
        type: "boolean", required: false
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
