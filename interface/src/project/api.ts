import { AxiosPromise } from "axios";

import { AXIOS } from "../api/endpoints";
import { LightMqttSettings, LightState, DeviceSettings, DeviceLoRaWanSettings, DeviceInformation } from "./types";

export function readLightState(): AxiosPromise<LightState> {
  return AXIOS.get('/lightState');
}

export function updateLightState(lightState: LightState): AxiosPromise<LightState> {
  return AXIOS.post('/lightState', lightState);
}

export function readBrokerSettings(): AxiosPromise<LightMqttSettings> {
  return AXIOS.get('/brokerSettings');
}

export function updateBrokerSettings(lightMqttSettings: LightMqttSettings): AxiosPromise<LightMqttSettings> {
  return AXIOS.post('/brokerSettings', lightMqttSettings);
}

export function readDeviceSettings(): AxiosPromise<DeviceSettings> {
  return AXIOS.get('/deviceSettings');
}

export function updateDeviceSettings(lightMqttSettings: DeviceSettings): AxiosPromise<DeviceSettings> {
  return AXIOS.post('/deviceSettings', lightMqttSettings);
}

export function readDeviceLoRaWanSettings(): AxiosPromise<DeviceLoRaWanSettings> {
  return AXIOS.get('/deviceLoraWanSettings');
}

export function updateDeviceLoRaWanSettings(lightMqttSettings: DeviceLoRaWanSettings): AxiosPromise<DeviceLoRaWanSettings> {
  return AXIOS.post('/deviceLoraWanSettings', lightMqttSettings);
}

export function readDeviceInfo(): AxiosPromise<DeviceInformation> {
  return AXIOS.get('/deviceState');
}

export function updateDeviceInfo(deviceInfo: DeviceInformation): AxiosPromise<DeviceInformation> {
  return AXIOS.post('/deviceState', deviceInfo);
}
