# Some importart change from react frame work.

- In interface/framework/SecuritySettingsService.cpp  change 
  - line 18: *from:* AsyncWebHeader* authorizationHeader = ... *to:* const AsyncWebHeader* authorizationHeader = ...
  - line 26: *from:* AsyncWebParameter* tokenParamater = ... *to:* const AsyncWebParameter* tokenParamater = ...

- Board
  We can use GPIO0 as an output to turn an external device on or off. Warning: during boot, it must be held high or left floating.