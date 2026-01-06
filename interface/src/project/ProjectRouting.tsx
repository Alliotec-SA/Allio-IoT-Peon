import { FC } from 'react';
import { Navigate, Routes, Route } from 'react-router-dom';

import Home from './Home';
import DeviceCommunicationSettings from './DeviceCommunicationSettings';

const ProjectRouting: FC = () => {
  return (
    <Routes>
      {
        // Add the default route for your project below
      }
      <Route path="/*" element={<Navigate to="home" />} />
      {
        // Add your project page routes below.
      }
      <Route path="home/*" element={<Home />} />
      <Route path="deviceCommunicationSettings/*" element={<DeviceCommunicationSettings />} />
    </Routes>
  );
};

export default ProjectRouting;
