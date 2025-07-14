export interface DeviceSettings {
  dev_eui: string;
  token: string;
  server: string;
  path: string;
}

export interface DeviceLoRaWanSettings {
  use_otaa: boolean;
  dev_eui: string;
  app_eui: string;
  app_key: string;
  net_key: string;
  dev_address: string;
  nets_key: string;
  apps_key: string;
}

export interface LightState {
  led_on: boolean;
}

export interface LightMqttSettings {
  unique_id: string;
  name: string;
  mqtt_path: string;
}
