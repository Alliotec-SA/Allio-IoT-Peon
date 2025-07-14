import { FC, useState } from "react";
import { ValidateFieldsError } from "async-validator";

import { Checkbox, Button } from "@mui/material";
import SaveIcon from '@mui/icons-material/Save';

import { BlockFormControlLabel, ButtonRow, FormLoader, MessageBox, SectionContent, ValidatedTextField } from "../components";
import { validate } from "../validators";
import { useRest, updateValue } from "../utils";

import * as DemoApi from './api';
import { DeviceSettings } from "./types";
import { DEVICE_SETTINGS_VALIDATOR } from "./validators";

const DeviceSettingsForm: FC = () => {
  const [fieldErrors, setFieldErrors] = useState<ValidateFieldsError>();
  const {
    loadData, saveData, saving, setData, data, errorMessage
  } = useRest<DeviceSettings>({ read: DemoApi.readDeviceSettings, update: DemoApi.updateDeviceSettings });

  const updateFormValue = updateValue(setData);

  const content = () => {
    if (!data) {
      return (<FormLoader onRetry={loadData} errorMessage={errorMessage} />);
    }

    const validateAndSubmit = async () => {
      try {
        setFieldErrors(undefined);
        await validate(DEVICE_SETTINGS_VALIDATOR, data);
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
              name="enabled"
              checked={data.enabled}
              onChange={updateFormValue}
            />
          }
          label={data.enabled ? "Uncheck to disable":"Check to enable"}
        />
        {data.enabled ? <>
            <ValidatedTextField
          fieldErrors={fieldErrors}
          name="dev_eui"
          label="EUI"
          fullWidth
          variant="outlined"
          value={data.dev_eui}
          onChange={updateFormValue}
          margin="normal"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="token"
          label="Token"
          fullWidth
          variant="outlined"
          value={data.token}
          onChange={updateFormValue}
          margin="normal"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="server"
          label="Server Domain"
          fullWidth
          variant="outlined"
          value={data.server}
          onChange={updateFormValue}
          margin="normal"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="path"
          label="Server Path"
          fullWidth
          variant="outlined"
          value={data.path}
          onChange={updateFormValue}
          margin="normal"
        />
        </> : <></>}
        
        <ButtonRow mt={1}>
          <Button startIcon={<SaveIcon />} disabled={saving} variant="contained" color="primary" type="submit" onClick={validateAndSubmit}>
            Save
          </Button>
        </ButtonRow>
      </>
    );
  };

  return (
    <SectionContent title='Device API Settings' titleGutter>
      {content()}
    </SectionContent>
  );
};

export default DeviceSettingsForm;
