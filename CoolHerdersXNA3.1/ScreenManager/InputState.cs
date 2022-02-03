//-----------------------------------------------------------------------------
// <copyright file="InputState.cs" company="Microsoft">
//  Microsoft XNA Community Game Platform
//  Copyright (C) Microsoft Corporation. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    #region Using Statements
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Input;
    #endregion

    /// <summary>
    /// Helper for reading input from keyboard and gamepad. This class tracks both
    /// the current and previous state of both input devices, and implements query
    /// properties for high level input actions such as "move up through the menu"
    /// or "pause the game".
    /// </summary>
    public class InputState : CoolHerders.IInputState
    {
        #region Fields

        /// <summary>
        /// This holds the maximum number of input sources we will accept
        /// </summary>
        public const int MaxInputs = 4;

        /// <summary>
        /// This holds the current keyboard states for all attached keyboards
        /// </summary>
        internal KeyboardState[] CurrentKeyboardStates;

        /// <summary>
        /// This holds the current gamepad states for all attached gamepads
        /// </summary>
        internal GamePadState[] CurrentGamePadStates;

        /// <summary>
        /// This holds the previous keyboard states for all attached keyboards
        /// </summary>
        internal KeyboardState[] LastKeyboardStates;

        /// <summary>
        /// This holds the previous gamepad states for all attached gamepads
        /// </summary>
        internal GamePadState[] LastGamePadStates;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the InputState class.
        /// </summary>
        public InputState()
        {
            this.CurrentKeyboardStates = new KeyboardState[MaxInputs];
            this.CurrentGamePadStates = new GamePadState[MaxInputs];

            this.LastKeyboardStates = new KeyboardState[MaxInputs];
            this.LastGamePadStates = new GamePadState[MaxInputs];
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets a value indicating whether a "menu up" input action is present from any player,
        /// on either keyboard or gamepad.
        /// </summary>
        public bool MenuUp
        {
            get
            {
                return this.IsNewKeyPress(Keys.Up) ||
                       this.IsNewButtonPress(Buttons.DPadUp) ||
                       this.IsNewButtonPress(Buttons.LeftThumbstickUp);
            }
        }

        /// <summary>
        /// Gets a value indicating whether a "menu down" input action is present from any player,
        /// on either keyboard or gamepad.
        /// </summary>
        public bool MenuDown
        {
            get
            {
                return this.IsNewKeyPress(Keys.Down) ||
                       this.IsNewButtonPress(Buttons.DPadDown) ||
                       this.IsNewButtonPress(Buttons.LeftThumbstickDown);
            }
        }

        /// <summary>
        /// Gets a value indicating whether a "menu select" input action is present from any player,
        /// on either keyboard or gamepad.
        /// </summary>
        public bool MenuSelect
        {
            get
            {
                return this.IsNewKeyPress(Keys.Space) ||
                       this.IsNewKeyPress(Keys.Enter) ||
                       this.IsNewButtonPress(Buttons.A) ||
                       this.IsNewButtonPress(Buttons.Start);
            }
        }

        /// <summary>
        /// Gets a value indicating whether a "menu cancel" input action is present from any player,
        /// on either keyboard or gamepad.
        /// </summary>
        public bool MenuCancel
        {
            get
            {
                return this.IsNewKeyPress(Keys.Escape) ||
                       this.IsNewButtonPress(Buttons.B) ||
                       this.IsNewButtonPress(Buttons.Back);
            }
        }

        /// <summary>
        /// Gets a value indicating whether a "pause the game" input action is present from any player,
        /// on either keyboard or gamepad.
        /// </summary>
        public bool PauseGame
        {
            get
            {
                return this.IsNewKeyPress(Keys.Escape) ||
                       this.IsNewButtonPress(Buttons.Back) ||
                       this.IsNewButtonPress(Buttons.Start);
            }
        }

        #endregion

        #region Methods

        /// <summary>
        /// Reads the latest state of the keyboard and gamepad.
        /// </summary>
        public void Update()
        {
            for (int i = 0; i < MaxInputs; i++)
            {
                this.LastKeyboardStates[i] = this.CurrentKeyboardStates[i];
                this.LastGamePadStates[i] = this.CurrentGamePadStates[i];

                this.CurrentKeyboardStates[i] = Keyboard.GetState((PlayerIndex)i);
                this.CurrentGamePadStates[i] = GamePad.GetState((PlayerIndex)i);
            }
        }

        /// <summary>
        /// Helper for checking if a key was newly pressed during this update,
        /// by any player.
        /// </summary>
        /// <param name="key">A Keys enum indicating the key to check for</param>
        /// <returns>A bool indicating if the key specified is newly pressed</returns>
        public bool IsNewKeyPress(Keys key)
        {
            for (int i = 0; i < MaxInputs; i++)
            {
                if (this.IsNewKeyPress(key, (PlayerIndex)i))
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Helper for checking if a key was newly pressed during this update,
        /// by the specified player.
        /// </summary>
        /// <param name="key">A Keys enum indicating the key to scan for</param>
        /// <param name="playerIndex">A PlayerIndex enum indicating the player to check</param>
        /// <returns>A boolean set to true of the specified key is pressed by the specified player</returns>
        public bool IsNewKeyPress(Keys key, PlayerIndex playerIndex)
        {
            return this.CurrentKeyboardStates[(int)playerIndex].IsKeyDown(key) &&
                    this.LastKeyboardStates[(int)playerIndex].IsKeyUp(key);
        }

        /// <summary>
        /// Helper for checking if a button was newly pressed during this update,
        /// by any player.
        /// </summary>
        /// <param name="button">A Buttons enum indicating the button to check for</param>
        /// <returns>A boolean which is true if the specified button was pressed</returns>
        public bool IsNewButtonPress(Buttons button)
        {
            for (int i = 0; i < MaxInputs; i++)
            {
                if (this.IsNewButtonPress(button, (PlayerIndex)i))
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Helper for checking if a button was newly pressed during this update,
        /// by the specified player.
        /// </summary>
        /// <param name="button">A Buttons enum indicating the button to scan for</param>
        /// <param name="playerIndex">A PlayerIndex enum indicating the player to scan</param>
        /// <returns>A boolean that is true if the specified button was pressed</returns>
        public bool IsNewButtonPress(Buttons button, PlayerIndex playerIndex)
        {
            return this.CurrentGamePadStates[(int)playerIndex].IsButtonDown(button) &&
                    this.LastGamePadStates[(int)playerIndex].IsButtonUp(button);
        }

        /// <summary>
        /// Checks for a "menu select" input action from the specified player.
        /// </summary>
        /// <param name="playerIndex">Index of the player to check</param>
        /// <returns>A boolean which is true if a menu selection is being made</returns>
        public bool IsMenuSelect(PlayerIndex playerIndex)
        {
            return this.IsNewKeyPress(Keys.Space, playerIndex) ||
                   this.IsNewKeyPress(Keys.Enter, playerIndex) ||
                   this.IsNewButtonPress(Buttons.A, playerIndex) ||
                   this.IsNewButtonPress(Buttons.Start, playerIndex);
        }

        /// <summary>
        /// Checks for a "menu cancel" input action from the specified player.
        /// </summary>
        /// <param name="playerIndex">Index of the player to check</param>
        /// <returns>A boolean which is true if the menu is being cancelled</returns>
        public bool IsMenuCancel(PlayerIndex playerIndex)
        {
            return this.IsNewKeyPress(Keys.Escape, playerIndex) ||
                   this.IsNewButtonPress(Buttons.B, playerIndex) ||
                   this.IsNewButtonPress(Buttons.Back, playerIndex);
        }

        #endregion
    }
}
