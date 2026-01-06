import { FC } from 'react';

import { List } from '@mui/material';
import SettingsRemoteIcon from '@mui/icons-material/SettingsRemote';
import Home from '@mui/icons-material/Home';

import { PROJECT_PATH } from '../api/env';
import LayoutMenuItem from '../components/layout/LayoutMenuItem';

const ProjectMenu: FC = () => (
  <List>
    <LayoutMenuItem icon={Home} label="Home" to={`/${PROJECT_PATH}/home`} />
    <LayoutMenuItem icon={SettingsRemoteIcon} label="Device Settings" to={`/${PROJECT_PATH}/deviceCommunicationSettings`} />
  </List>
);

export default ProjectMenu;
