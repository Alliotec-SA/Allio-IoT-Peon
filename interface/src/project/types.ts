export interface DeviceSettings {
  dev_eui: string;
  token: string;
  server: string;
  path: string;
}

export interface LightState {
  led_on: boolean;
}

export interface LightMqttSettings {
  unique_id: string;
  name: string;
  mqtt_path: string;
}
