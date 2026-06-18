export const LORAWAN_BANDS: { value: number; label: string }[] = [
  { value: 0, label: "EU433" },
  { value: 1, label: "CN470" },
  { value: 2, label: "RU864" },
  { value: 3, label: "IN865" },
  { value: 4, label: "EU868" },
  { value: 5, label: "US915" },
  { value: 6, label: "AU915" },
  { value: 7, label: "KR920" },
  { value: 8, label: "AS923" }
];

export const DEFAULT_LORAWAN_RADIO = {
  band: 5,
  sub_band: 2,
  data_rate: 3,
  rx2_dr: 8,
  rx2_freq_hz: 923300000,
  adr: true,
  confirm_mode: false
};

export type Rx2FreqOption = { value: number; label: string };

export const RX2_FREQ_OPTIONS: Record<number, Rx2FreqOption[]> = {
  0: [{ value: 433525000, label: "433.525 MHz" }],
  1: [{ value: 505300000, label: "505.3 MHz" }],
  2: [{ value: 869100000, label: "869.1 MHz" }],
  3: [{ value: 866550000, label: "866.55 MHz" }],
  4: [{ value: 869525000, label: "869.525 MHz" }],
  5: [{ value: 923300000, label: "923.3 MHz" }],
  6: [{ value: 923300000, label: "923.3 MHz" }],
  7: [{ value: 921900000, label: "921.9 MHz" }],
  8: [
    { value: 923200000, label: "923.2 MHz (AS923-1)" },
    { value: 923400000, label: "923.4 MHz (AS923-2)" }
  ]
};

export const RX2_DEFAULTS: Record<number, { rx2_dr: number; rx2_freq_hz: number }> = {
  0: { rx2_dr: 0, rx2_freq_hz: 433525000 },
  1: { rx2_dr: 0, rx2_freq_hz: 505300000 },
  2: { rx2_dr: 0, rx2_freq_hz: 869100000 },
  3: { rx2_dr: 0, rx2_freq_hz: 866550000 },
  4: { rx2_dr: 0, rx2_freq_hz: 869525000 },
  5: { rx2_dr: 8, rx2_freq_hz: 923300000 },
  6: { rx2_dr: 8, rx2_freq_hz: 923300000 },
  7: { rx2_dr: 0, rx2_freq_hz: 921900000 },
  8: { rx2_dr: 0, rx2_freq_hz: 923200000 }
};

export const SUB_BAND_OPTIONS = [1, 2, 3, 4, 5, 6, 7, 8, 9];
export const DATA_RATE_OPTIONS = [0, 1, 2, 3, 4, 5, 6, 7];

export function bandUsesSubBand(band: number): boolean {
  return band === 1 || band === 5 || band === 6;
}

export function getRx2FreqOptionsForBand(band: number): Rx2FreqOption[] {
  return RX2_FREQ_OPTIONS[band] ?? RX2_FREQ_OPTIONS[5];
}

export function getRx2DrOptionsForBand(band: number): number[] {
  if (band === 5 || band === 6) {
    return [8];
  }
  return [0, 1, 2, 3, 4, 5, 6, 7];
}

export function normalizeLoRaWanRadioSettings<T extends Partial<typeof DEFAULT_LORAWAN_RADIO>>(data: T) {
  return {
    ...DEFAULT_LORAWAN_RADIO,
    ...data
  };
}
