import { FC, useState } from "react";
import { ValidateFieldsError } from "async-validator";

import {
  Accordion,
  AccordionDetails,
  AccordionSummary,
  Button,
  Checkbox,
  RadioGroup,
  FormControlLabel,
  Radio,
  FormControl,
  MenuItem,
  Typography,
  Box
} from "@mui/material";
import ExpandMoreIcon from "@mui/icons-material/ExpandMore";
import SaveIcon from '@mui/icons-material/Save';

import { BlockFormControlLabel, ButtonRow, FormLoader, MessageBox, SectionContent, ValidatedTextField } from "../components";
import { validate } from "../validators";
import { useRest, updateValue } from "../utils";

import * as DemoApi from './api';
import { DeviceLoRaWanSettings } from "./types";
import { DEVICE_LORAWAN_SETTINGS_VALIDATOR } from "./validators";
import {
  bandUsesSubBand,
  DATA_RATE_OPTIONS,
  getRx2DrOptionsForBand,
  getRx2FreqOptionsForBand,
  LORAWAN_BANDS,
  normalizeLoRaWanRadioSettings,
  RX2_DEFAULTS,
  SUB_BAND_OPTIONS
} from "./lorawanRadio";

const accordionSx = {
  "&:before": { display: "none" },
  border: "1px solid",
  borderColor: "divider",
  borderRadius: 1,
  boxShadow: "none",
  "&.Mui-expanded": { margin: 0 }
};

const settingsStackSx = {
  display: "flex",
  flexDirection: "column",
  gap: 2,
  mt: 2,
  width: "100%"
};

const DeviceLoRaWanSettingsForm: FC = () => {
  const [fieldErrors, setFieldErrors] = useState<ValidateFieldsError>();
  const [connectionExpanded, setConnectionExpanded] = useState(true);
  const [radioExpanded, setRadioExpanded] = useState(false);
  const {
    loadData, save, saving, setData, data, errorMessage
  } = useRest<DeviceLoRaWanSettings>({
    read: DemoApi.readDeviceLoRaWanSettings,
    update: DemoApi.updateDeviceLoRaWanSettings
  });

  const updateFormValue = updateValue(setData);

  const content = () => {
    if (!data) {
      return (<FormLoader onRetry={loadData} errorMessage={errorMessage} />);
    }

    const settings = normalizeLoRaWanRadioSettings(data);
    const rx2FreqOptions = getRx2FreqOptionsForBand(settings.band);
    const rx2DrOptions = getRx2DrOptionsForBand(settings.band);
    const rx2FreqValue = rx2FreqOptions.some((o) => o.value === settings.rx2_freq_hz)
      ? settings.rx2_freq_hz
      : rx2FreqOptions[0].value;

    const validateAndSubmit = async () => {
      try {
        setFieldErrors(undefined);
        const payload = normalizeLoRaWanRadioSettings(data);
        await validate(DEVICE_LORAWAN_SETTINGS_VALIDATOR, payload);
        await save(payload);
      } catch (errors: any) {
        setFieldErrors(errors);
      }
    };

    const updateNumericField = (name: keyof DeviceLoRaWanSettings) => (
      (event: React.ChangeEvent<HTMLInputElement>) => {
        const value = Number(event.target.value);
        setData((prev) => prev ? { ...prev, [name]: value } : prev);
      }
    );

    const updateBand = (event: React.ChangeEvent<HTMLInputElement>) => {
      const band = Number(event.target.value);
      const rx2Defaults = RX2_DEFAULTS[band];
      setData((prev) => prev ? {
        ...prev,
        band,
        ...(rx2Defaults ?? {})
      } : prev);
    };

    const updateAdr = (event: React.ChangeEvent<HTMLInputElement>) => {
      const adr = event.target.value === "1";
      setData((prev) => prev ? { ...prev, adr } : prev);
    };

    const updateConfirmMode = (event: React.ChangeEvent<HTMLInputElement>) => {
      const confirm_mode = event.target.value === "1";
      setData((prev) => prev ? { ...prev, confirm_mode } : prev);
    };

    return (
      <>
        <MessageBox
          level="info"
          message="Get device connection information from PEON or Alliotec"
          my={2}
        />
        <BlockFormControlLabel
          control={
            <Checkbox
              name="enabled"
              checked={settings.enabled}
              onChange={updateFormValue}
            />
          }
          label={settings.enabled ? "Uncheck to disable" : "Check to enable"}
        />

        {settings.enabled && (
          <Box sx={settingsStackSx}>
            <Accordion
              expanded={connectionExpanded}
              onChange={(_, expanded) => setConnectionExpanded(expanded)}
              sx={accordionSx}
            >
              <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                <Typography fontWeight={500}>Connection / Keys</Typography>
              </AccordionSummary>
              <AccordionDetails sx={{ pt: 0, px: { xs: 1.5, sm: 2 }, pb: 2 }}>
                <BlockFormControlLabel
                  control={
                    <Checkbox
                      name="use_otaa"
                      checked={settings.use_otaa}
                      onChange={updateFormValue}
                    />
                  }
                  label={settings.use_otaa ? "Uncheck to use ABP" : "Check to use OTAA"}
                />

                {settings.use_otaa ? (
                  <>
                    <ValidatedTextField
                      fieldErrors={fieldErrors}
                      name="dev_eui"
                      label="Device EUI"
                      fullWidth
                      variant="outlined"
                      value={settings.dev_eui}
                      onChange={updateFormValue}
                      margin="normal"
                    />
                    <ValidatedTextField
                      fieldErrors={fieldErrors}
                      name="app_eui"
                      label="Application EUI"
                      fullWidth
                      variant="outlined"
                      value={settings.app_eui}
                      onChange={updateFormValue}
                      margin="normal"
                    />
                    <ValidatedTextField
                      fieldErrors={fieldErrors}
                      name="app_key"
                      label="Application Key"
                      fullWidth
                      variant="outlined"
                      value={settings.app_key}
                      onChange={updateFormValue}
                      margin="normal"
                    />
                  </>
                ) : (
                  <>
                    <ValidatedTextField
                      fieldErrors={fieldErrors}
                      name="dev_address"
                      label="Device Address"
                      fullWidth
                      variant="outlined"
                      value={settings.dev_address}
                      onChange={updateFormValue}
                      margin="normal"
                    />
                    <ValidatedTextField
                      fieldErrors={fieldErrors}
                      name="nets_key"
                      label="Network Session Key"
                      fullWidth
                      variant="outlined"
                      value={settings.nets_key}
                      onChange={updateFormValue}
                      margin="normal"
                    />
                    <ValidatedTextField
                      fieldErrors={fieldErrors}
                      name="apps_key"
                      label="Application Session Key"
                      fullWidth
                      variant="outlined"
                      value={settings.apps_key}
                      onChange={updateFormValue}
                      margin="normal"
                    />
                  </>
                )}

                <FormControl component="fieldset" sx={{ mt: 2, width: "100%" }}>
                  <Typography variant="body2" color="text.secondary" sx={{ mb: 1 }}>
                    Class Mode
                  </Typography>
                  <RadioGroup
                    name="class_mode"
                    value={settings.class_mode}
                    onChange={updateFormValue}
                    sx={{
                      flexDirection: { xs: "column", sm: "row" },
                      gap: { xs: 0, sm: 1 }
                    }}
                  >
                    <FormControlLabel value="A" control={<Radio />} label="Class A" />
                    <FormControlLabel value="B" control={<Radio />} label="Class B" />
                    <FormControlLabel value="C" control={<Radio />} label="Class C" />
                  </RadioGroup>
                </FormControl>
              </AccordionDetails>
            </Accordion>

            <Accordion
              expanded={radioExpanded}
              onChange={(_, expanded) => setRadioExpanded(expanded)}
              sx={accordionSx}
            >
              <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                <Typography fontWeight={500}>Advanced Radio</Typography>
              </AccordionSummary>
              <AccordionDetails sx={{ pt: 0, px: { xs: 1.5, sm: 2 }, pb: 2 }}>
                <ValidatedTextField
                  fieldErrors={fieldErrors}
                  name="band"
                  label="Region"
                  fullWidth
                  variant="outlined"
                  value={settings.band}
                  onChange={updateBand}
                  margin="normal"
                  select
                >
                  {LORAWAN_BANDS.map(({ value, label }) => (
                    <MenuItem key={value} value={value}>{label}</MenuItem>
                  ))}
                </ValidatedTextField>

                {bandUsesSubBand(settings.band) && (
                  <ValidatedTextField
                    fieldErrors={fieldErrors}
                    name="sub_band"
                    label="Sub-band"
                    fullWidth
                    variant="outlined"
                    value={settings.sub_band}
                    onChange={updateNumericField("sub_band")}
                    margin="normal"
                    select
                  >
                    {SUB_BAND_OPTIONS.map((value) => (
                      <MenuItem key={value} value={value}>{value}</MenuItem>
                    ))}
                  </ValidatedTextField>
                )}

                <ValidatedTextField
                  fieldErrors={fieldErrors}
                  name="adr"
                  label="Adaptive Data Rate (ADR)"
                  fullWidth
                  variant="outlined"
                  value={settings.adr ? "1" : "0"}
                  onChange={updateAdr}
                  margin="normal"
                  select
                >
                  <MenuItem value="0">Off</MenuItem>
                  <MenuItem value="1">On</MenuItem>
                </ValidatedTextField>

                <ValidatedTextField
                  fieldErrors={fieldErrors}
                  name="data_rate"
                  label="Data Rate (DR)"
                  fullWidth
                  variant="outlined"
                  value={settings.data_rate}
                  onChange={updateNumericField("data_rate")}
                  margin="normal"
                  select
                  disabled={settings.adr}
                  helperText={settings.adr ? "Managed by ADR when enabled" : "Used when ADR is off"}
                >
                  {DATA_RATE_OPTIONS.map((value) => (
                    <MenuItem key={value} value={value}>DR{value}</MenuItem>
                  ))}
                </ValidatedTextField>

                <ValidatedTextField
                  fieldErrors={fieldErrors}
                  name="confirm_mode"
                  label="Default confirm mode (CFM)"
                  fullWidth
                  variant="outlined"
                  value={settings.confirm_mode ? "1" : "0"}
                  onChange={updateConfirmMode}
                  margin="normal"
                  select
                >
                  <MenuItem value="0">Off (unconfirmed)</MenuItem>
                  <MenuItem value="1">On (confirmed)</MenuItem>
                </ValidatedTextField>

                <ValidatedTextField
                  fieldErrors={fieldErrors}
                  name="rx2_dr"
                  label="RX2 Data Rate"
                  fullWidth
                  variant="outlined"
                  value={settings.rx2_dr}
                  onChange={updateNumericField("rx2_dr")}
                  margin="normal"
                  select
                >
                  {rx2DrOptions.map((value) => (
                    <MenuItem key={value} value={value}>DR{value}</MenuItem>
                  ))}
                </ValidatedTextField>

                <ValidatedTextField
                  fieldErrors={fieldErrors}
                  name="rx2_freq_hz"
                  label="RX2 Frequency"
                  fullWidth
                  variant="outlined"
                  value={rx2FreqValue}
                  onChange={updateNumericField("rx2_freq_hz")}
                  margin="normal"
                  select
                >
                  {rx2FreqOptions.map(({ value, label }) => (
                    <MenuItem key={value} value={value}>{label}</MenuItem>
                  ))}
                </ValidatedTextField>
              </AccordionDetails>
            </Accordion>
          </Box>
        )}

        <ButtonRow mt={2}>
          <Button startIcon={<SaveIcon />} disabled={saving} variant="contained" color="primary" type="submit" onClick={validateAndSubmit}>
            Save
          </Button>
        </ButtonRow>
      </>
    );
  };

  return (
    <SectionContent title='Device LoRaWAN Settings' titleGutter>
      {content()}
    </SectionContent>
  );
};

export default DeviceLoRaWanSettingsForm;
