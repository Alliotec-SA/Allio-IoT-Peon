import { FC } from 'react';
import { Navigate, Routes, Route } from 'react-router-dom';

import DeviceCommunicationSettings from './DeviceCommunicationSettings';

const ProjectRouting: FC = () => {
  return (
    <Routes>
      {
        // Add the default route for your project below
      }
      <Route path="/*" element={<Navigate to="deviceCommunicationSettings" />} />
      {
        // Add your project page routes below.
      }
      <Route path="deviceCommunicationSettings/*" element={<DeviceCommunicationSettings />} />
    </Routes>
  );
};

export default ProjectRouting;
