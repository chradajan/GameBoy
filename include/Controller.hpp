#pragma once

struct Buttons
{
    bool down, up, left, right;  // Directions
    bool start, select, b, a; // Actions
};

Buttons GetButtons();
