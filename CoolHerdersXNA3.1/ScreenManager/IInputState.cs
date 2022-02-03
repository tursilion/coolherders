//-----------------------------------------------------------------------------
// <copyright file="IInputState.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------
namespace CoolHerders
{
    using System;

    /// <summary>
    /// This is the interface to the Input State system so that any class in the game can see what the controllers are doing.
    /// </summary>
    internal interface IInputState
    {
        /// <summary>
        /// Gets a value indicating whether the menu is being cancelled
        /// </summary>
        bool MenuCancel
        {
            get;
        }

        /// <summary>
        /// Gets a value indicating whether the menu selector should move down
        /// </summary>
        bool MenuDown
        {
            get;
        }

        /// <summary>
        /// Gets a value indicating whether the menu selector action should be taken
        /// </summary>
        bool MenuSelect
        {
            get;
        }

        /// <summary>
        /// Gets a value indicating whether the menu selector should move up
        /// </summary>
        bool MenuUp
        {
            get;
        }

        /// <summary>
        /// Gets a value indicating whether the game should pause
        /// </summary>
        bool PauseGame
        {
            get;
        }

        /// <summary>
        /// Determines if a player is selecting cancel at the menu
        /// </summary>
        /// <param name="playerIndex">Index of the player's gamepad to check</param>
        /// <returns>True if the player is selecting cancel, false otherwise</returns>
        bool IsMenuCancel(Microsoft.Xna.Framework.PlayerIndex playerIndex);

        /// <summary>
        /// Determines if a player is selecting a menu option
        /// </summary>
        /// <param name="playerIndex">Index of the player's gamepad to check</param>
        /// <returns>True if the player is selecting an option, false otherwise</returns>
        bool IsMenuSelect(Microsoft.Xna.Framework.PlayerIndex playerIndex);

        /// <summary>
        /// Is a button newly pressed on any game controller
        /// </summary>
        /// <param name="button">The button ID to scan for</param>
        /// <returns>True if the button is pressed, false otherwise</returns>
        bool IsNewButtonPress(Microsoft.Xna.Framework.Input.Buttons button);

        /// <summary>
        /// Is a button newly pressed on a given game controller
        /// </summary>
        /// <param name="button">The button ID to scan for</param>
        /// <param name="playerIndex">Index of the player's gamepad to check</param>
        /// <returns>True if the given player is pressing the given button, false otherwise</returns>
        bool IsNewButtonPress(Microsoft.Xna.Framework.Input.Buttons button, Microsoft.Xna.Framework.PlayerIndex playerIndex);

        /// <summary>
        /// Is a key newly pressed on any keyboard
        /// </summary>
        /// <param name="key">The key ID to scan for</param>
        /// <returns>True if the given key is pressed, false otherwise</returns>
        bool IsNewKeyPress(Microsoft.Xna.Framework.Input.Keys key);

        /// <summary>
        /// Is a key newly pressed on a given player's keyboard
        /// </summary>
        /// <param name="key">The key ID to scan for</param>
        /// <param name="playerIndex">Index of the player's keyboard to check</param>
        /// <returns>True if the given player is pressing the given key, false otherwise</returns>
        bool IsNewKeyPress(Microsoft.Xna.Framework.Input.Keys key, Microsoft.Xna.Framework.PlayerIndex playerIndex);

        /// <summary>
        /// Called to update the controller status
        /// </summary>
        void Update();
    }
}
