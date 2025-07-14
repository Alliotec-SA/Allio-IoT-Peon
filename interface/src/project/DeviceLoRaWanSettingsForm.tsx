import { FC, useState } from "react";
import { ValidateFieldsError } from "async-validator";

import { Button, Checkbox } from "@mui/material";
import SaveIcon from '@mui/icons-material/Save';

import { BlockFormControlLabel, ButtonRow, FormLoader, MessageBox, SectionContent, ValidatedTextField } from "../components";
import { validate } from "../validators";
import { useRest, updateValue } from "../utils";

import * as DemoApi from './api';
import { DeviceLoRaWanSettings } from "./types";
import { DEVICE_LORAWAN_SETTINGS_VALIDATOR } from "./validators";

const DeviceLoRaWanSettingsForm: FC = () => {
  const [fieldErrors, setFieldErrors] = useState<ValidateFieldsError>();
  const {
    loadData, saveData, saving, setData, data, errorMessage
  } = useRest<DeviceLoRaWanSettings>({ read: DemoApi.readDeviceLoRaWanSettings, update: DemoApi.updateDeviceLoRaWanSettings });

  const updateFormValue = updateValue(setData);

  const content = () => {
    if (!data) {
      return (<FormLoader onRetry={loadData} errorMessage={errorMessage} />);
    }

    const validateAndSubmit = async () => {
      try {
        setFieldErrors(undefined);
        await validate(DEVICE_LORAWAN_SETTINGS_VALIDATOR, data);
        saveData();
      } catch (errors: any) {
        setFieldErrors(errors);
      }
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
              name="use_otaa"
              checked={data.use_otaa}
              onChange={updateFormValue}
            />
          }
          label={data.use_otaa ? "Uncheck to use ABP":"Check to use OTAA"}
        />
        {data.use_otaa ? <div><ValidatedTextField
          fieldErrors={fieldErrors}
          name="dev_eui"
          label="Device EUI"
          fullWidth
          variant="outlined"
          value={data.dev_eui}
          onChange={updateFormValue}
          margin="normal"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="app_eui"
          label="Application EUI"
          fullWidth
          variant="outlined"
          value={data.app_eui}
          onChange={updateFormValue}
          margin="normal"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="app_key"
          label="Application Key"
          fullWidth
          variant="outlined"
          value={data.app_key}
          onChange={updateFormValue}
          margin="normal"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="net_key"
          label="Network Key"
          fullWidth
          variant="outlined"
          value={data.net_key}
          onChange={updateFormValue}
          margin="normal"
        /></div> : <>
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="dev_address"
          label="Device Address"
          fullWidth
          variant="outlined"
          value={data.dev_address}
          onChange={updateFormValue}
          margin="normal"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="nets_key"
          label="Network Session Key"
          fullWidth
          variant="outlined"
          value={data.nets_key}
          onChange={updateFormValue}
          margin="normal"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="apps_key"
          label="Application Session Key"
          fullWidth
          variant="outlined"
          value={data.apps_key}
          onChange={updateFormValue}
          margin="normal"
        />
        </>}
        
        
        <ButtonRow mt={1}>
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
