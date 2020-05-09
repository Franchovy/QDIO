/*
  ==============================================================================

    MenuItem.h
    Created: 9 May 2020 3:55:20pm
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class MenuItem
{
public:
    MenuItem(int numMenus);

    void addMenuItem(int menuIndex, PopupMenu::Item newItem);
    int callMenu(int menuIndex);

private:
    OwnedArray<PopupMenu> menus;
};