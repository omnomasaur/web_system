
#pragma once

#include <Windows.h>

//------------------------------------------------------------------------
//Input Simulation Section
//------------------------------------------------------------------------
//The following classes are for creating mouse input event simulation in windows. 
//They require Windows.h be included.
//Credit for them goes to: golanshahar of http://forums.codeguru.com/showthread.php?t=377394
//He is awesome.
void MouseMove (int x, int y )
{  
  double fScreenWidth    = ::GetSystemMetrics( SM_CXSCREEN )-1; 
  double fScreenHeight  = ::GetSystemMetrics( SM_CYSCREEN )-1; 
  double fx = x*(65535.0f/fScreenWidth);
  double fy = y*(65535.0f/fScreenHeight);
  INPUT  Input={0};
  Input.type      = INPUT_MOUSE;
  Input.mi.dwFlags  = MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE;
  Input.mi.dx = (LONG)fx;
  Input.mi.dy = (LONG)fy;
  ::SendInput(1,&Input,sizeof(INPUT));
}

void LeftClick ( )
{  
  INPUT    Input={0};
  // left down 
  Input.type      = INPUT_MOUSE;
  Input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
  ::SendInput(1,&Input,sizeof(INPUT));

  // left up
  ::ZeroMemory(&Input,sizeof(INPUT));
  Input.type      = INPUT_MOUSE;
  Input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
  ::SendInput(1,&Input,sizeof(INPUT));
}

void RightClick ( )
{  
  INPUT    Input={0};
  // right down 
  Input.type      = INPUT_MOUSE;
  Input.mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;
  ::SendInput(1,&Input,sizeof(INPUT));

  // right up
  ::ZeroMemory(&Input,sizeof(INPUT));
  Input.type      = INPUT_MOUSE;
  Input.mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
  ::SendInput(1,&Input,sizeof(INPUT));
}
//------------------------------------------------------------------------
//End Input Simulation Section
//------------------------------------------------------------------------