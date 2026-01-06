export interface DeviceSettings {
  dev_eui: string;
  token: string;
  server: string;
  path: string;
  enabled: boolean;
}

export interface DeviceInformation {
  last_checked?: string;
  signal_voltage?: string;
  signa_period?: string;
  battery_voltage?: string;
  solar_voltage?: string;
  battery?: string;
  timeout?: string;
  is_running_analyzing_process?: boolean;
  is_turned_on?: boolean;
  is_ultra_energy_saving_mode?: boolean;
  start_analyzing_process?: boolean;
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
  enabled: boolean;
}

export interface LightState {
  led_on: boolean;
}

export interface LightMqttSettings {
  unique_id: string;
  name: string;
  mqtt_path: string;
}
