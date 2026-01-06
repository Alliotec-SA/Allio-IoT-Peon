import { FC, useState, useEffect } from "react";
import { ValidateFieldsError } from "async-validator";

import { Checkbox, Button } from "@mui/material";
import { PlayCircle, BrowserUpdated, StopCircle } from "@mui/icons-material";

import { LoadingSpinner } from "../components";

import { BlockFormControlLabel, ButtonRow, FormLoader, MessageBox, SectionContent, ValidatedTextField } from "../components";
import { validate } from "../validators";
import { useRest, updateValue } from "../utils";

import * as DemoApi from './api';
import { DeviceInformation } from "./types";
import { DEVICE_SETTINGS_VALIDATOR } from "./validators";

const MainTab: FC = () => {
  const [fieldErrors, setFieldErrors] = useState<ValidateFieldsError>();
  const {
    loadData, saveData, saving, setData, data, errorMessage
  } = useRest<DeviceInformation>({ read: DemoApi.readDeviceInfo, update: DemoApi.updateDeviceInfo });

  const [isRunningAnalyzingProcess, setIsRunningAnalyzingProcess] = useState<boolean>(false);


  const updateFormValue = updateValue(setData);


  useEffect(() => {
      let interval: NodeJS.Timeout; 
      if (isRunningAnalyzingProcess) {
        interval = setInterval(async () => {
          await loadData();
          if (!data?.is_running_analyzing_process) {
            setIsRunningAnalyzingProcess(false);
            clearInterval(interval);
          }
        }, 5000);
      }
      return () => clearInterval(interval);
    }, [isRunningAnalyzingProcess, data]);

  const content = () => {
    
    if (isRunningAnalyzingProcess ||data?.is_running_analyzing_process) {
      return (<LoadingSpinner variant="h6" size={50} height={"5%"} message="Waiting for analyzer to finish..." />);
    }

    if (!data) {
      return (<FormLoader onRetry={loadData} errorMessage={errorMessage} />);
    }

  

    const validateAndSubmit = async (deviceValues:DeviceInformation) => {
      try {
        DemoApi.updateDeviceInfo(deviceValues);
        await loadData();
      } catch (errors: any) {
        setFieldErrors(errors);
      }
    };

    return (
      <>
     
            <ValidatedTextField
          fieldErrors={fieldErrors}
          name="dev_eui"
          label="Last updated"
          fullWidth
          variant="outlined"
          value={data.last_checked}
          InputProps={{
            readOnly: true
          }}
          margin="normal"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="token"
          label="Peak Voltage (v)"
          fullWidth
          variant="outlined"
          value={data.signal_voltage}
          InputProps={{
            readOnly: true
          }}
          margin="normal"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="server"
          label="Peak Period Interval (s)"
          fullWidth
          variant="outlined"
          value={(data.signa_period ? (parseInt(data.signa_period)/1000).toFixed(2) : "")}
          InputProps={{
            readOnly: true
          }}
          margin="normal"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="path"
          label="Peon Battery Voltage (v)"
          fullWidth
          variant="outlined"
          value={data.battery_voltage}
          InputProps={{
            readOnly: true
          }}
          margin="normal"
        />

        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="path"
          label="Peon Panel Voltage (v)"
          fullWidth
          variant="outlined"
          value={data.solar_voltage}
          InputProps={{
            readOnly: true
          }}
          margin="normal"
        />

        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="path"
          label="Device Battery Voltage (v)"
          fullWidth
          variant="outlined"
          value={data.battery}
          InputProps={{
            readOnly: true
          }}
          margin="normal"
        />

        
        <ButtonRow mt={1}>
          <Button startIcon={ <PlayCircle />} disabled={saving} variant="contained" color="primary" type="submit" onClick={()=>{validateAndSubmit({is_running_analyzing_process:true})}}>
            Start Analysis 
          </Button>
          <Button startIcon={<BrowserUpdated />} disabled={saving} variant="contained" color="primary" type="submit" onClick={()=> loadData()}>
            Reload 
          </Button>
          <Button startIcon={data.is_turned_on ? <StopCircle /> : <PlayCircle />} disabled={saving} variant="contained" color={data.is_turned_on ? "error" : "success"} type="submit" onClick={()=>{validateAndSubmit({is_turned_on:!data.is_turned_on})}}>
            Turn {data.is_turned_on ? "Off" : "On"}
          </Button>
        </ButtonRow>
      </>
    );
  };

  return (
    <SectionContent title='Values' titleGutter>
      {content()}
    </SectionContent>
  );
};

export default MainTab;
