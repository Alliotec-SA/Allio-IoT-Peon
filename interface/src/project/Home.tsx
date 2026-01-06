
import React, { FC } from 'react';
import { Navigate, Route, Routes } from 'react-router-dom';

import { Tab } from '@mui/material';

import { RouterTabs, useRouterTab, useLayoutTitle } from '../components';
import MainTab from './MainTab';

const Home: FC = () => {
  useLayoutTitle("Home");
  const { routerTab } = useRouterTab();

  return (
    <>
      <RouterTabs value={routerTab}>
          <Tab value="main" label="Main" />
        </RouterTabs>
        <Routes>
          <Route path="main" element={<MainTab />} />
          <Route path="/*" element={<Navigate replace to="main" />} />
        </Routes>
    </>
  );
};

export default Home;
